#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "util.h"

#define STATM_SIZE 46

/*
 * Get the maximum value
 */
double max(double a, double b) {
	if (a > b)
		return a;
	return b;
}

/*
 * Return a random float between zero and 1
 */
double rand_float() {
	return (rand())/(RAND_MAX+1.0);
}


/*
 * Normalize the input value
 */
double normalize(double value, double max, double min) {
	return (value-min)/(max-min);
}


int undo_normalize(double value, double max, double min) {
	return (int)(value*(max-min)) + min;
}


/*
 * Filter function used by scandir() function in get_procs() function
 */
int filter(const struct dirent * entry) {
	if ((*entry->d_name > '0') && (*entry->d_name <= '9'))
		return 1;

	return 0;
}


/*
 * Get the list of pids and command name
 * Return the size of list
 */
int get_procs(struct proc *** list) {
	struct proc ** proclist;
	struct dirent ** namelist;
	int i, n;
	char filename[25];
	FILE * fp;
	ssize_t bytes_read;
	size_t len = 0;
	char * line = NULL;

	n = scandir("/proc", &namelist, filter, versionsort);

	proclist = malloc(n * sizeof(struct proc));		

	if (n < 0) {
		fprintf(stderr, "error scanning /proc: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	else {
		for(i=0; i<n; i++) {
			proclist[i] = (struct proc *)malloc(sizeof(struct proc));
			sscanf(namelist[i]->d_name, "%u", &proclist[i]->pid);
			free(namelist[i]);

			snprintf(filename,
					sizeof(filename),
					"/proc/%d/status",
					proclist[i]->pid);

			fp = fopen(filename, "r");

			if (fp == NULL) {
				fprintf(stderr,
						"error opening file %s: %s\n",
						filename, strerror(errno));
				exit(EXIT_FAILURE);
			}

			bytes_read = getline(&line, &len, fp);

			if (bytes_read != -1)
				sscanf(line, "Name: %s", proclist[i]->cmd);

			if (fclose(fp)) {
				fprintf(stderr,
						"error closing file %s: %s\n",
						filename, strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		free(namelist);
	}

	*list = proclist;
	return i;
}


/*
 * Free the list of applications
 */
void free_procs(struct proc *** list, int length) {
	struct proc ** proclist = *list;
	int i;

	for (i=0; i<length; i++)
		free(proclist[i]);

	free(proclist);

	*list = NULL;
}


/*
 * Read the number of rss pages
 */
unsigned int read_statm(pid_t pid) {
	int fd;
	char filename[25];
	char statm[STATM_SIZE];
	size_t length;
	unsigned int vm_pages, rss_pages;

	snprintf(filename, sizeof(filename), "/proc/%d/statm", (int)pid);

	fd = open(filename, O_RDONLY);

	if (fd == -1) {
		fprintf(stderr,
				"error opening file %s: %s\n",
				filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	length = read(fd, statm, sizeof(statm));
	close(fd);

	statm[length] = '\0';

	sscanf(statm, "%u %u", &vm_pages, &rss_pages);

	return rss_pages;
}


struct rss_list * read_log_statm(char * filename, unsigned long * length) {
	FILE * fp;
	ssize_t bytes_read;
	size_t len = 0;
	char * line = NULL;
	unsigned long index;
	unsigned int vm_pages, rss_pages;
	struct rss_list head, * list;

	list = NULL;

	fp = fopen(filename, "r");

	if (fp == NULL) {
		fprintf(stderr,
				"error opening file %s: %s\n",
				filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	head.next = NULL;
	list = &head;

	while ((bytes_read = getline(&line, &len, fp)) != -1) {
		sscanf(line, "%lu %u %u", &index, &vm_pages, &rss_pages);
		list->next = (struct rss_list *)malloc(sizeof(struct rss_list));
		list = list->next;
		list->rss_pages = rss_pages;
		list->next = NULL;
	}

	if (line)
		free(line);

	if (fclose(fp)) {
		fprintf(stderr,
				"error closing file %s: %s\n",
				filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	*length = index + 1;

	return head.next;
}


void free_rss_list(struct rss_list * list) {
	struct rss_list * temp;
	while (list) {
		temp = list;
		list = list->next;
		free(temp);
	}
}


char * get_classname(double value, double max, double min, char * flag) {
	double avg;

	if (value >= 0) {
		avg = max/3;
		if (value < avg) {
			strcpy(flag, "Low");
		}
		else if (value < (avg*2)) {
			strcpy(flag, "Medium");
		}
		else {
			strcpy(flag, "High");
		}
	}
	else if (value < 0) {
		avg = min/3;
		if (value > avg) {
			strcpy(flag, "-Low");
		}
		else if (value > (avg*2)) {
			strcpy(flag, "-Medium");
		}
		else {
			strcpy(flag, "-High");
		}
	}

	return flag;
}



int get_classposition(double value, double max, double min) {
	double avg;
	int position = 0 ; 
	if (value >= 0) {
		avg = max/3;
		if (value < avg) {
			position= 3 ; 
		}
		else if (value < (avg*2)) {
			position = 4 ; 
		}
		else {
			position = 5; 
		}
	}
	else if (value < 0) {
		avg = min/3;
		if (value > avg) {
			position= 0; 
		}
		else if (value > (avg*2)) {
			position = 1; 
		}
		else {
			position= 2; 
		}
	}
	return position; 

}
