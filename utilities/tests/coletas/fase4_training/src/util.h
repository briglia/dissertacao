#include <stdlib.h>
//Parameters used for XEON 
/*
#define MEM_MAX 62069.0
#define MEM_MIN 0.0
#define VELOC_MAX 2305.0
#define VELOC_MIN -5570.0
#define ACEL_MAX 5712.0
#define ACEL_MIN -5570.0
*/
//Parameters for OMAP 
#define MEM_MAX 6126.0
#define MEM_MIN 0.0
#define VELOC_MAX 748
#define VELOC_MIN -2148.0
#define ACEL_MAX 2148.0
#define ACEL_MIN -2148.0


struct proc {
	pid_t pid;
	char cmd[30];
};

struct rss_list {
	unsigned int rss_pages;
	struct rss_list * next;
};

double max(double, double);

double rand_float(void);

double normalize(double, double, double);

int undo_normalize(double, double, double);

int get_procs(struct proc***);

void free_procs(struct proc***, int);

unsigned int read_statm(pid_t);

struct rss_list * read_log_statm(char*, unsigned long*);

void free_rss_list(struct rss_list *);

char * get_classname(double, double, double, char*);
