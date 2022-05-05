// Functional Decomposition vs Data Decomposition
// Andrea Tongsak
// CS 475: Parallel Programming

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <iostream>

#ifndef DEBUG
#define DEBUG false
#endif

omp_lock_t			Lock;
volatile int		NumInThreadTeam;
volatile int		NumAtBarrier;
volatile int		NumGone;

const float GRAIN_GROWS_PER_MONTH = 9.0; // play with this value
const float ONE_DEER_EATS_PER_MONTH = 1.0; // change?
const float AVG_PRECIP_PER_MONTH = 7.0;	// average
const float AMP_PRECIP_PER_MONTH = 6.0;	// plus or minus
const float RANDOM_PRECIP = 2.0;	// plus or minus noise
const float AVG_TEMP = 60.0;	// average, in Farenheit
const float AMP_TEMP = 20.0;	// plus or minus
const float RANDOM_TEMP = 10.0;	// plus or minus noise
const float MIDTEMP = 40.0;
const float MIDPRECIP = 10.0;

// "State" of the system: following global variables
int	NowYear;		// 2022 - 2027
int	NowMonth;		// 0 - 11
float	NowPrecip;		// inches of rain per month
float	NowTemp;		// temperature this month
float	NowHeight;		// grain height in inches
int	NowNumDeer;		// number of deer in the current population
int NowNumHunters; // number of hunters

// Function Declaration
void InitBarrier(int);
void WaitBarrier();

float Ranf(unsigned int* seed, float low, float high);
int Ranf(unsigned int* seed, int low, int high);
float SQR(float x);

void Weather();
void Watcher();
void Deer();
void Grain();
void Hunter();

unsigned int seed = 0;
float x = Ranf(&seed, -1.f, 1.f);

void initialize() {
	// starting date and time:
	NowMonth = 0;
	NowYear = 2022;

	// starting state:
	NowNumDeer = 1;
	NowHeight = 1;
	NowNumHunters = 0;
	
	Weather();
}

int main() {

#ifndef _OPENMP
	fprintf(stderr, "OpenMP is not supported - sorry!\n");
	exit(0);
#endif
//	TimeofDaySeed();

	initialize();

	InitBarrier(4);
	omp_set_num_threads(4);	// same as # of sections
#pragma omp parallel sections
	{
#pragma omp section
		{
			Deer();
		}

#pragma omp section
		{
			Grain();
		}

#pragma omp section
		{
			Hunter(); // your own
		}

#pragma omp section
		{
			Watcher();
		}

	}       // implied barrier -- all functions must return in order
			// to allow any of them to get past here
	return 0;
}

float Ranf(unsigned int* seedp, float low, float high)
{
//rand(*seedp);
	float r = (float)rand();          // 0 - RAND_MAX

	return(low + r * (high - low) / (float)RAND_MAX);
}

int Ranf(unsigned int* seedp, int ilow, int ihigh)
{
	float low = (float)ilow;
	float high = (float)ihigh + 0.9999f;

	return (int)(Ranf(seedp, low, high));
}

float SQR(float x)
{
	return x * x;
}

void Weather() {
	// temperature and precipitation
	float ang = (30. * (float)NowMonth + 15.) * (M_PI / 180.);

	float temp = AVG_TEMP - AMP_TEMP * cos(ang);
	NowTemp = temp + Ranf(&seed, -RANDOM_TEMP, RANDOM_TEMP);

	float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
	NowPrecip = precip + Ranf(&seed, -RANDOM_PRECIP, RANDOM_PRECIP);

	if (NowPrecip < 0.) {
		NowPrecip = 0.;
	}
}

void Deer() {
	while (NowYear < 2028) {
		if (DEBUG) {
			fprintf(stderr, "NowYear: %d", NowYear);
		}

		int nextNumDeer = NowNumDeer;
		int carryingCapacity = (int)(NowHeight);
		if (nextNumDeer < carryingCapacity) {
			nextNumDeer++;
		}
		else {
			if (nextNumDeer > carryingCapacity)
				nextNumDeer--;
		}
		if (nextNumDeer < 0) {
			nextNumDeer = 0;
		}

		// Decrease deer pop. with increase of hunter pop.
		nextNumDeer -= (NowNumHunters / 2);


		// DoneComputing
		WaitBarrier();
		if (DEBUG) {
			fprintf(stderr, "nextYearDeer: %d\n", nextNumDeer);
		}
		NowNumDeer = nextNumDeer;

		// DoneAssigning
		WaitBarrier();

		// DonePrinting
		WaitBarrier();
	}
}

void Grain() {
	while (NowYear < 2028) {
		float tempFactor = exp(-SQR((NowTemp - MIDTEMP) / 10.));
		float precipFactor = exp(-SQR((NowPrecip - MIDPRECIP) / 10.));

		float nextHeight = NowHeight;
		nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
		nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
		if (nextHeight < 0.) nextHeight = 0.;

		if (DEBUG) {
			fprintf(stderr, "nextHeight: %.f\n", nextHeight);
		}

		// DoneComputing
		WaitBarrier();
		if (DEBUG) {
			fprintf(stderr, "nextHeight: %.f\n", nextHeight);
		}

		NowHeight = nextHeight;

		// DoneAssigning
		WaitBarrier();

		// DonePrinting
		WaitBarrier();
	}
}

void Watcher() {

	printf("Year,Month,Temp(f),Precip(in),Grain Ht(in),Deer Pop.,Hunter Pop.\n");

	while (NowYear < 2028) {
		// DoneComputing
		WaitBarrier();
		// DoneAssigning
		WaitBarrier();
		printf("%d,%d,%.5f,%.5f,%.5f,%d,%d\n", NowYear, NowMonth, NowTemp, NowPrecip, NowHeight, NowNumDeer, NowNumHunters);

		// increment month
		NowMonth++;

		if (NowMonth > 11) {
			NowMonth = 0;
			NowYear++;
		}

		Weather();

		// DonePrinting
		WaitBarrier();
	}
}

void Hunter() {
	while (NowYear < 2028) {
		int nextNumHunters = NowNumDeer / 5;

		if (nextNumHunters < 0) {
			nextNumHunters = 0;
		}

		// DoneComputing
		WaitBarrier();
		NowNumHunters = nextNumHunters;

		// DoneAssigning
		WaitBarrier();

		// DonePrinting
		WaitBarrier();
	}
}

void InitBarrier(int n)
{
	NumInThreadTeam = n;
	NumAtBarrier = 0;
	omp_init_lock(&Lock);
}

void WaitBarrier()
{
	omp_set_lock(&Lock);
	{
		NumAtBarrier++;
		if (NumAtBarrier == NumInThreadTeam) {
			NumGone = 0;
			NumAtBarrier = 0;
			// let all other threads return before this one unlocks:
			while (NumGone != NumInThreadTeam - 1);
			omp_unset_lock(&Lock);
			return;
		}
	}
	omp_unset_lock(&Lock);

	while (NumAtBarrier != 0);	// this waits for the nth thread to arrive

	#pragma omp atomic
	NumGone++;		// flags how many threads have returned
}
