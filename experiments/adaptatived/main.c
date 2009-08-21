#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "util.h"
#include "som.h"


/*
 * Read the input from file for Self Organizing Maps (SOM)
 * and return the length of inputs
 */
int read_som_input(struct input_pattern * inputs[], int maxlen) {
	char filename[] = "input.txt";
	FILE * fp;
	ssize_t bytes_read;
	size_t len = 0;
	char * line = NULL;
	unsigned int pages;
	int veloc, acel;
	int i = 0;

	fp = fopen(filename, "r");

	if (fp == NULL) {
		fprintf(stderr,
				"error opening file %s: %s\n",
				filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	maxlen--;

	while((bytes_read = getline(&line, &len, fp)) != -1) {
		if (maxlen < i) {
			fprintf(stdout,
					"input array is not large enough "
					"to fit the input file: %u < %u\n",
					maxlen, i);
			break;
		}

		sscanf(line, "(%u, %d, %d)", &pages, &veloc, &acel);
		inputs[i] = (struct input_pattern *)
			malloc(sizeof(struct input_pattern));
		inputs[i]->weights[0] = normalize((double)pages, 
				MEM_MAX, 
				MEM_MIN);
		inputs[i]->weights[1] = normalize((double)veloc, 
				VELOC_MAX, 
				VELOC_MIN);
		inputs[i]->weights[2] = normalize((double)acel, 
				ACEL_MAX, 
				ACEL_MIN);
		i++;
	};

	if (line)
		free(line);

	if (fclose(fp)) {
		fprintf(stderr,
				"error closing file %s: %s\n",
				filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	return i;
}


/*
 * Read the trained Self Organizing Maps (SOM) from filename
 */
void read_trained_som(char filename[], struct som_node * grids[][GRIDS_YSIZE]) {
	FILE * fp;
	ssize_t bytes_read;
	size_t len = 0;
	char * line = NULL;
	char * input[WEIGHTS_SIZE];
	int i, j;

	fp = fopen(filename, "r");

	if (fp == NULL) {
		fprintf(stderr,
				"error opening file %s: %s\n",
				filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	for (i=0; i<WEIGHTS_SIZE; i++) 
		input[i] = (char *)malloc(10);

	while((bytes_read = getline(&line, &len, fp)) != -1) {
		sscanf(line, "(%d, %d) %s %s %s",
				&i, &j, input[0], input[1], input[2]);

		grids[i][j] = (struct som_node *)
			malloc(sizeof(struct som_node));
		grids[i][j]->xp = i;
		grids[i][j]->yp = j;
		grids[i][j]->weights[0] = strtod(input[0], NULL);
		grids[i][j]->weights[1] = strtod(input[1], NULL);
		grids[i][j]->weights[2] = strtod(input[2], NULL);
	}

	for (i=0; i<WEIGHTS_SIZE; i++)
		free(input[i]);

	if (line)
		free(line);

	if (fclose(fp)) {
		fprintf(stderr,
				"error closing file %s: %s\n",
				filename, strerror(errno));
		exit(EXIT_FAILURE);
	}
}


/*
 * Save the trained Self Organizing Maps (SOM) to filename
 */
void save_trained_som(char filename[], struct som_node * grids[][GRIDS_YSIZE]) {
	FILE * fp;
	int i, j;
	char * line;

	fp = fopen(filename, "w");

	if (fp == NULL) {
		fprintf(stderr,
				"error opening file %s: %s\n",
				filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	for (i=0; i<GRIDS_XSIZE; i++) {
		for (j=0; j<GRIDS_YSIZE; j++) {
			line = (char *)malloc(50);
			sprintf(line, "(%d, %d) %f %f %f\n",
					i, j,
					grids[i][j]->weights[0],
					grids[i][j]->weights[1],
					grids[i][j]->weights[2]);
			fputs(line, fp);
			free(line);
		}
	}

	if (fclose(fp)) {
		fprintf(stderr,
				"error closing file %s: %s\n",
				filename, strerror(errno));
		exit(EXIT_FAILURE);
	}
}


/*
 * Allocate and initiate all nodes in the grid with their
 * weights set randomly
 */
void init_grid(struct som_node * grids[][GRIDS_YSIZE]) {
	int i, j, k;
	srand(time(0));
	for (i=0; i<GRIDS_XSIZE; i++) {
		for (j=0; j<GRIDS_YSIZE; j++) {
			grids[i][j] = (struct som_node *)
				malloc(sizeof(struct som_node));
			grids[i][j]->xp = i;
			grids[i][j]->yp = j;
			for  (k=0; k<WEIGHTS_SIZE; k++) {
				grids[i][j]->weights[k] = rand_float();
			}
		}
	}
}


/*
 * Print all nodes with their weights from the grid for 
 * debugging purpose
 */
void print_grid(struct som_node * grids[][GRIDS_YSIZE]) {
	int i, j;
	for (i=0; i<GRIDS_YSIZE; i++) {
		printf("\n\n");
		for (j=0; j<GRIDS_XSIZE; j++) {
			printf("(%d %d %d)",
					(int)(grids[i][j]->weights[0] * 255),
					(int)(grids[i][j]->weights[1] * 255),
					(int)(grids[i][j]->weights[2] * 255)
			      );
		}
	}
}


/*
 * Train Kohonen's Self Organizing Map (SOM).
 * This training code is faster
 */
void train(struct input_pattern * inputs[], int input_len) {
	int lw = GRIDS_XSIZE;
	int lh = GRIDS_YSIZE;
	int xstart, ystart, xend, yend;
	double dist, influence;
	double grid_radius = max(lw, lh)/2;
	double time_constant = NUM_ITERATIONS / log(grid_radius);

	int iteration = 0;
	double nbh_radius;
	struct som_node * bmu = NULL;
	struct som_node * temp = NULL;
	double * new_input = NULL;
	double learning_rate = START_LEARNING_RATE;
	int i, x, y;

	time_t start_time = 0;
	time_t end_time = 0;
	/* Train NUM_ITERATIONS times */
	while (iteration < NUM_ITERATIONS) {
		time(&start_time);

		/* Get the neighborhood radius */
		nbh_radius = neighborhood_radius(grid_radius,
				iteration,
				time_constant);
		/* For each input pattern, determine the BMU and adjust */
		/* the weights for the BMU's neighborhood */
		for (i=0; i<input_len; i++) {
			new_input = inputs[i]->weights;
			/* Get the BMU */
			bmu = get_bmu(new_input, WEIGHTS_SIZE, grids);

			xstart = (int)(bmu->xp - nbh_radius - 1);
			ystart = (int)(bmu->yp - nbh_radius - 1);
			xend = (int)(bmu->xp + nbh_radius + 1);
			yend = (int)(bmu->yp + nbh_radius + 1);
			if (xend > lw)
				xend = lw;
			if (xstart < 0)
				xstart = 0;
			if (yend > lh)
				yend = lh;
			if (ystart < 0)
				ystart = 0;

			/* Optimization:  Only go through the (x,y) values */ 
			/* that fall within the radius */
			for (x=xstart; x<xend; x++) {
				for (y=ystart; y<yend; y++) {
					temp = grids[x][y];
					dist = distance_to(bmu, temp);
					if (dist <= (nbh_radius * nbh_radius)) {
						influence = get_influence
							(dist, nbh_radius);
						adjust_weights
							(temp,
							 inputs[i],
							 WEIGHTS_SIZE,
							 learning_rate,
							 influence);
					}
				}
			}
		}
		iteration++;
		/* Calculate the learning rate */
		learning_rate = START_LEARNING_RATE *
			exp(-(double)iteration/NUM_ITERATIONS);

		time(&end_time);
		printf("Iteration..: %d -> %f s\n", 
				iteration, 
				(difftime(end_time, start_time)));
	}
}


/*
 * Train Kohonen's Self Organizing Map (SOM).
 * This training code is slower than the first one
 */
void train2(struct input_pattern * inputs[], int input_len) {
	int lw = GRIDS_XSIZE;
	int lh = GRIDS_YSIZE;
	double dist, influence;
	double grid_radius = max(lw, lh)/2;
	double time_constant = NUM_ITERATIONS / log(grid_radius);

	int iteration = 0;
	double nbh_radius;
	struct som_node * bmu = NULL;
	struct som_node * temp = NULL;
	double * new_input = NULL;
	double learning_rate = START_LEARNING_RATE;
	int i, x, y;

	/* Train NUM_ITERATIONS times */
	while (iteration < NUM_ITERATIONS) {
		/* Get the neighborhood radius */
		nbh_radius = neighborhood_radius(grid_radius,
				iteration,
				time_constant);
		/* For each input pattern, determine the BMU and adjust */
		/* the weights for the BMU's neighborhood */
		for (i=0; i<input_len; i++) {
			new_input = inputs[i]->weights;
			/* Get the BMU */
			bmu = get_bmu(new_input, WEIGHTS_SIZE, grids);

			for (x=0; x<GRIDS_XSIZE; x++) {
				for (y=0; y<GRIDS_XSIZE; y++) {
					temp = grids[x][y];
					dist = distance_to(bmu, temp);
					if (dist <= (nbh_radius * nbh_radius)) {
						influence = get_influence
							(dist, nbh_radius);
						adjust_weights
							(temp,
							 inputs[i],
							 WEIGHTS_SIZE,
							 learning_rate,
							 influence);
					}
				}
			}
		}
		iteration++;
		/* Calculate the learning rate */
		learning_rate = START_LEARNING_RATE *
			exp(-(double)iteration/NUM_ITERATIONS);
	}
}


struct som_node * get_bmu_xy (struct som_node * grids[][GRIDS_YSIZE],
		unsigned int * new_rss,
		unsigned int * current_rss,
		int * veloc,
		int * accel) {
	int temp_veloc;
	struct input_pattern * neural_input = NULL;
	struct som_node * bmu = NULL;

	/* Update the current rss pages, velocity and acceleration */
	temp_veloc = *new_rss - *current_rss;
	*accel = temp_veloc - *veloc;
	*veloc = temp_veloc;
	*current_rss = *new_rss;

	neural_input = (struct input_pattern *)
		malloc(sizeof(struct input_pattern));

	neural_input->weights[0] = normalize((double)*current_rss,
			MEM_MAX,
			MEM_MIN);
	neural_input->weights[1] = normalize((double)*veloc, 
			VELOC_MAX, 
			VELOC_MIN);
	neural_input->weights[2] = normalize((double)*accel, 
			ACEL_MAX, 
			ACEL_MIN);

	/* Get the BMU */
	bmu = get_bmu(neural_input->weights, WEIGHTS_SIZE, grids);

	free(neural_input);

	return bmu;
}

/*
   int main (int argc, char * argv[]) {	
   init_grid(grids);

   print_grid(grids);

   struct input_pattern * inputs[5];
   inputs[0] = (struct input_pattern *)
   malloc(sizeof(struct input_pattern));
   inputs[0]->weights[0] = 0.1;
   inputs[0]->weights[1] = 0.25;
   inputs[0]->weights[2] = 0.015;

   inputs[1] = (struct input_pattern *)
   malloc(sizeof(struct input_pattern));
   inputs[1]->weights[0] = 0.5;
   inputs[1]->weights[1] = 0.35;
   inputs[1]->weights[2] = 0.2;

   inputs[2] = (struct input_pattern *)
   malloc(sizeof(struct input_pattern));
   inputs[2]->weights[0] = 0.58;
   inputs[2]->weights[1] = 0.75;
   inputs[2]->weights[2] = 0.4;

   inputs[3] = (struct input_pattern *)
   malloc(sizeof(struct input_pattern));
   inputs[3]->weights[0] = 0.066;
   inputs[3]->weights[1] = 0.99;
   inputs[3]->weights[2] = 0.01;

   inputs[4] = (struct input_pattern *)
   malloc(sizeof(struct input_pattern));
   inputs[4]->weights[0] = 0.326;
   inputs[4]->weights[1] = 0.888;
   inputs[4]->weights[2] = 0.271;


   clock_t start, end;
   double elapsed;
   start = clock();

   train(inputs, 5);

   end = clock();
   elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
   printf("Time Elapsed: %f\n\n", elapsed);

   start = clock();


//train2(inputs, 5);


end = clock();
elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
printf("Time Elapsed: %f\n\n", elapsed);


printf("\n\n***********\n\n");
print_grid(grids);

return 0;
}
*/
