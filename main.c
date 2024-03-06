#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "rtclock.h"
#include "mmm.h"

// shared  globals
unsigned int mode;
unsigned int size, num_threads;
double **A, **B, **SEQ_MATRIX, **PAR_MATRIX;

typedef struct {
	int startRow;
	int endRow;
} Threadz;

int main(int argc, char *argv[]) {

	// TODO - deal with command line arguments, save the "mode"
	// "size" and "num threads" into globals so threads can see them

	// if less than 3 arguments, print an error with hint about proper syntax
	if (argc < 3) {
		printf("Usage: ./mmmSol S <size>\n");
		printf("Usage: ./mmmSol P <num threads> <size>\n");
		exit(1);
	}
	// check if the first argument is either S or P
	else if (strcmp(argv[1], "S") != 0 && strcmp(argv[1], "P") != 0) {
		printf("Error: mode must be either S (sequential) or P (parallel)\n");
		exit(1);
	}
	// if P, there should be 4 arguments
	else if (strcmp(argv[1], "P") == 0 && argc < 4) {
		printf("Error: parallel mode requires [num threads]\n");
		exit(1);
	}

	// save the size and num threads into global vars
	if (strcmp(argv[1], "S") == 0) {
		mode = 0; // 0 for Sequential Mode
		int s = atoi(argv[2]);
		if (s < 0) {
			printf("Size must be a positive integer");
			exit(1);
		}
		else {
			size = s; // make the size a global variable
		}

		printf("========\n");
		printf("mode: sequential\n");
		printf("thread count: 1\n");
		printf("size: %d\n", size);
		printf("========\n");
	}
	else {
		mode = 1; // 1 for Parallel Mode
		int t = atoi(argv[2]);
		int s = atoi(argv[3]);
		if (s < 0 || t < 0) {
			printf("Size and number of threads must be positive integers");
			exit(1);
		}
		else {
			num_threads = t; // make the number of threads a global variable
			size = s; // make the size a global variable
		}

		printf("========\n");
		printf("mode: parallel\n");
		printf("thread count: %d\n", num_threads);
		printf("size: %d\n", size);
		printf("========\n");
	}



	// initialize my matrices
	mmm_init();
	double clockstart, clockend;

	// << stuff I want to clock here >>
	if (mode == 0) {
		double seqTime = 0;
		
		// run 4 times to smoothen the results
		for (int i = 0; i < 4; i++) {
			clockstart = rtclock();	// start the clock
			mmm_seq(); // run sequential
			clockend = rtclock(); // stop the clock

			// throw away 1st run and add the times of all other runs
			if (i != 0) seqTime += clockend - clockstart;
			mmm_reset(SEQ_MATRIX);
		}

		printf("Sequential Time taken (avg of 3 runs): %.6f sec\n", seqTime/3);
	}
	else {
		// sequential run
		double seqTime = 0;
		
		// run 4 times to smoothen the results
		for (int i = 0; i < 4; i++) {
			clockstart = rtclock();	// start the clock
			mmm_seq(); // run sequential
			clockend = rtclock(); // stop the clock

			// throw away 1st run and add the times of all other runs
			if (i != 0) seqTime += clockend - clockstart;
			mmm_reset(SEQ_MATRIX);
		}

		printf("Sequential Time taken (avg of 3 runs): %.6f sec\n", seqTime/3);


		// parallel run
		double parTime = 0;

		// run 4 times to smoothen the results
		for (int i = 0; i < 4; i++) {
			clockstart = rtclock();	// start the clock
			// create the threads here and make each of them call the mmm_par function
			pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
			Threadz *mmthread = malloc(num_threads * sizeof(Threadz));

			int rowsPerThread = size / num_threads;
			int initialStartRow = 0;
			int leftovers = size % num_threads; // handling case where it isn't evenly divisible

			// set up the start and end rows; create the threads
			for (int i = 0; i < num_threads; i++) {
				mmthread[i].startRow = initialStartRow;
				mmthread[i].endRow = initialStartRow + rowsPerThread - 1;

				// if leftovers, give one extra row to each thread
				if (leftovers > 0) {
					mmthread[i].endRow += 1;
					leftovers -= 1;
					initialStartRow += 1;
				}

				initialStartRow += rowsPerThread;
				pthread_create(&threads[i], NULL, mmm_par, &mmthread[i]);
			}

			// join the threads together
			for (int i = 0; i < num_threads; i++) {
				pthread_join(threads[i], NULL);
			}

			free(threads);
			free(mmthread);
			clockend = rtclock(); // stop the clock

			if(i != 0) parTime += clockend - clockstart;
			mmm_reset(PAR_MATRIX);
		}

		printf("Parallel Time (avg of 3 runs): %.6f sec\n", parTime/3);

		// print the speedup of parallel version over sequential
		double speedup = seqTime / parTime;
		printf("Speedup: %.6f\n", speedup);

		// print the largest error between parallel and sequential matrices
		double error = mmm_verify(); 
		printf("Verifying... largest error between parallel and sequential matrix: %.6f\n", error);
		
	}

	mmm_freeup();
	return 0;
}
