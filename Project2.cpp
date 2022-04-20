// Andrea Tongsak
// CS 475 Parallel Programming
// Project 2
// Numeric Integration with OpenMP Reduction
// "You're doing well, all things considered. :)"

// Superquadric
// x^N + y^N + z^N = R^N

// NUMNODES: at least 8 < 1000000, change values later
// 1, 2, 4

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <omp.h>

#define XMIN	 -1.
#define XMAX      1.
#define YMIN     -1.
#define YMAX      1.

#ifndef NUMT
#define NUMT 4
#endif

#ifndef NUMNODES
#define NUMNODES 45800
#endif

const float N = 2.5f;
const float R = 1.2f;

float Height(int, int);	// function prototype

int main(int argc, char* argv[])
{

#ifndef _OPENMP
	fprintf(stderr, "OpenMP is not supported, sorry!");
	return 1;
#endif

	omp_set_num_threads(NUMT);

	// the area of a single full-sized tile:
	// (not all tiles are full-sized, though)
	float fullTileArea = (((XMAX - XMIN) / (float)(NUMNODES - 1)) * ((YMAX - YMIN) / (float)(NUMNODES - 1)));

	float maximumPerformance = 0;
	float totalVolume = 0;

	for (int trial = 0; trial < NUMT; trial++) {
		double startTime = omp_get_wtime();

		float volume = 0;
#pragma omp parallel for default(none), shared(fullTileArea), reduction(+:volume)
		for (int i = 0; i < NUMNODES*NUMNODES; i++) {
			int iu = i % NUMNODES;
			int iv = i / NUMNODES;
			float z = Height(iu, iv);

			// check if iu and iv is on the edge
			bool iu_edge = (iu == 0 || iu == NUMNODES - 1);
			bool iv_edge = (iv == 0 || iv == NUMNODES - 1);

			if (iu_edge && iv_edge) {
				// Both dimensions on the edge
				volume += z * (fullTileArea/4);
			}
			else if (iu_edge || iv_edge) {
				// One dimension is on the edge
				volume += z * (fullTileArea / 2);
			}
			else {
				// There is a full tile area, or square here
				volume += z * (fullTileArea);
			}
		}

		double totalSeconds = omp_get_wtime() - startTime;
		double MegaHeightsPerSeconds = (double)(NUMNODES * NUMNODES) / totalSeconds / 1000000;

		if (MegaHeightsPerSeconds > maximumPerformance) {
			maximumPerformance = MegaHeightsPerSeconds;
		}

		totalVolume += volume;
	}

	float averageVolume = totalVolume / NUMT;

	printf("%d\t%d\t%.4lf\t%.4lf\n", NUMT, NUMNODES * NUMNODES, averageVolume, maximumPerformance);

	return 0;
}

float Height(int iu, int iv) // iu, iv = 0 ... NUMNODES-1
{
	float x = -1. + 2. * (float)iu / (float)(NUMNODES - 1);	// -1. to +1.
	float y = -1. + 2. * (float)iv / (float)(NUMNODES - 1);	// -1. to +1.

	float xn = pow(fabs(x), (double)N);
	float yn = pow(fabs(y), (double)N);
	float rn = pow(fabs(R), (double)N);
	float r = rn - xn - yn;

	if (r <= 0.)
		return 0.;

	float height = pow(r, 1. / (double)N);
	return height;
}
