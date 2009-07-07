#define WEIGHTS_SIZE 3
#define GRIDS_XSIZE 40
#define GRIDS_YSIZE 40
#define START_LEARNING_RATE 0.1
#define NUM_ITERATIONS 500
//#define INPUT_MAXSIZE 65300
#define INPUT_MAXSIZE 3819


struct som_node {
	double weights[WEIGHTS_SIZE];
	int xp;
	int yp;
};

struct som_node * grids[GRIDS_XSIZE][GRIDS_YSIZE];

struct input_pattern {
	double weights[WEIGHTS_SIZE];
};

struct input_pattern * inputs[INPUT_MAXSIZE];

double euclidean_dist(double*, double*, int);

double neighborhood_radius(double, double, double);

struct som_node * get_bmu(double*, int, struct som_node* [][GRIDS_YSIZE]);

double distance_to(struct som_node*, struct som_node*);

double get_influence(double, double);

void adjust_weights(struct som_node*, struct input_pattern*, 
		    int, double, double);
