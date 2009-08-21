#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "som.h"


/*
 * Calculate the Euclidean distance between each node's weights 
 * vector and the current input vector
 */
double euclidean_dist(double *input, double *weights, int len) {
	double summation = 0;
	double temp;
	int i;

	for (i=0; i<len; i++) {
		//printf("input: %f  weight: %f \n", input[i], weights[i]);
		temp = (input[i]-weights[i]) * (input[i]-weights[i]);
		summation += temp;
	}

	return summation;
}


/*
 * Calculate the neighbourhood radius for each iteration
 */
double neighborhood_radius(double init_radius, 
		double iteration, 
		double time_constant) {
	return init_radius * exp(-iteration/time_constant);
}


/*
 * Calculate the Best Matching Unit (BMU)
 */
struct som_node * get_bmu(double input_vector[],
		int len_input,
		struct som_node * grids[][GRIDS_YSIZE]
		) {
	struct som_node *bmu = grids[0][0];
	double best_dist = euclidean_dist(input_vector, 
			bmu->weights, 
			len_input);
	double new_dist;
	int i, j;

	for (i=0; i<GRIDS_XSIZE; i++) {
		for (j=0; j<GRIDS_YSIZE; j++) {
			new_dist = euclidean_dist(input_vector,
					grids[i][j]->weights,
					len_input);
			if (new_dist < best_dist) {
				//printf("%f < %f \n", new_dist, best_dist);
				bmu = grids[i][j];
				best_dist = new_dist;
			}
		}
	}
	return bmu;
}


/*
 * Distance between the node1 and node2
 */
double distance_to(struct som_node * node1, struct som_node * node2) {
	int x, y;
	x = node1->xp - node2->xp;
	x *= x;
	y = node1->yp - node2->yp;
	y *= y;
	return x + y;
}


/*
 * Calculate the amount of influence a node's distance from the BMU 
 * has on its learning
 */
double get_influence(double distance, double radius) {
	double radius_sq = radius * radius;
	return exp(-(distance)/(2 * radius_sq));
}


/*
 * The node within the BMU's neighbourhood has its weights vector adjusted
 */
void adjust_weights(struct som_node * node, 
		struct input_pattern * input,
		int len_input,
		double learning_rate, 
		double influence) {
	double node_weight, input_weight;
	int i;

	for (i=0; i<len_input; i++) {
		node_weight = node->weights[i];
		input_weight = input->weights[i];
		node_weight += influence * learning_rate * 
			(input_weight-node_weight);
		node->weights[i] = node_weight;
	}
}
