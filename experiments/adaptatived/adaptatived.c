#include <glib.h>
#include <unistd.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#include "som.h"
#include "util.h"
#include "ui.h"

#define MAX_INPUTS 90
#define MEM 131072

int mem_free_array[MAX_INPUTS];

extern int read_som_input(struct input_pattern* [], int);
int read_trained_som(char filename[], struct som_node * grids[][GRIDS_YSIZE]); 
extern void init_grid(struct som_node* [][GRIDS_YSIZE]);
extern void train(struct input_pattern* [], int);
extern void save_trained_som(char*, struct som_node* [][GRIDS_YSIZE]);
extern struct som_node * get_bmu_xy(struct som_node* [][GRIDS_YSIZE],
		unsigned int*,
		unsigned int*,
		int*, int*);

/* freq tables */
int freq_browser[40][40];
int freq_canola[40][40];
int freq_pdf[40][40];

int get_memfree()
{
	FILE* fp;
	char buffer[2048];
	size_t bytes_read;
	char* match;
	int MemFree;

	/* Read the entire contents of /proc/meminfo into the buffer.  */
	fp = fopen ("/proc/meminfo", "r");
	bytes_read = fread (buffer, 1, sizeof (buffer), fp);
	fclose (fp);

	/* Bail if read failed or if buffer isn't big enough.  */
	if (bytes_read == 0 || bytes_read == sizeof (buffer))
		return 0;

	/* NULL-terminate the text.  */
	buffer[bytes_read] = '\0';

	/* Locate the line that starts with "MemFree".  */
	match = strstr (buffer, "MemFree:");
	if (match == NULL)
		MemFree = 0;
	else
		sscanf(match, "MemFree: %d", &MemFree);

	return MEM - MemFree;
}

void populate_mem_free_array() {
	int i = 0;
	int timeout = 1;

	do {
		mem_free_array[i] = get_memfree();
		sleep(timeout);
		i++;
	} while(i<MAX_INPUTS);

}

void read_freq_log(char filename[]) {
	FILE *fp;
	ssize_t bytes_read;
	size_t len = 0;
	char * line = NULL;
	int i, j, freq;

	i = 0;
	j = 0;
	freq = 0;

	fp = fopen(filename, "r");

	if (fp == NULL) {
		printf("Error opening freq log file\n");
		return;
	}
	while((bytes_read = getline(&line, &len, fp)) != -1) {
		sscanf(line, "(%d, %d) %d", &i, &j, &freq);
		if (strcmp(filename, "freq-browser.log") == 0) {
			freq_browser[i][j] = freq;
		} else if (strcmp(filename, "freq-canola.log") == 0) {
			freq_canola[i][j] = freq;
		} else if (strcmp(filename, "freq-pdf.log") == 0) {
			freq_pdf[i][j] = freq;
		}
	}

	if (line)
		free(line);
	if (fclose(fp))
		printf("Error closing file\n");
}

struct rss_list * read_mem_free_log() {
	unsigned int vm_pages, rss_pages;
	struct rss_list head, *list;
	int i = 0;

	list = NULL;
	head.next = NULL;
	list = &head;

	for (i = 0; i < MAX_INPUTS; i++) {
		list->next = (struct rss_list *)malloc(sizeof(struct
					rss_list));
		list = list->next;
		list->rss_pages = mem_free_array[i];
		list->next = NULL;
	}

	return head.next;
}

int magic() {
	struct rss_list *head = NULL;
	struct rss_list *iterator = NULL;
	int x, i, j;
	unsigned int current_rss = 0;
	unsigned int new_rss;
	struct som_node *bmu = NULL;
	int veloc = 0;
	int accel = 0;
	int browser_factor = 0;
	int canola_factor = 0;
	int pdf_factor = 0;

	i = j = 0;
	/* Collect memFree for x seconds */
	populate_mem_free_array();

	/* Get the head pointer for rss_list */
	head = read_mem_free_log();
	iterator = head;

	for (x = 0; x < MAX_INPUTS; x++) {
		/* Logfile radion button is active */
		new_rss = iterator->rss_pages;
		iterator = iterator->next;

		/* Get the closest bmu */
		bmu = get_bmu_xy(grids,
				&new_rss,
				&current_rss,
				&veloc,
				&accel);
		i = bmu->xp;
		j = bmu->yp;

		if (freq_pdf[i][j] != 0) {
			pdf_factor = pdf_factor + freq_pdf[i][j];
		} else if (freq_browser[i][j] != 0) {
			browser_factor = browser_factor + freq_browser[i][j];
		} else if (freq_canola[i][j] != 0) {
			canola_factor = canola_factor + freq_canola[i][j];
		}
	}
	printf("PDF factor: %d\n", pdf_factor);
	printf("Browser factor: %d\n", browser_factor);
	printf("Canola factor: %d\n", canola_factor);
}

int main()
{
	int i;
	int j;

	/* Save trained som in grids */
	//read_trained_som("saved_som.txt", grids);

	/* Init frequency matrix for browser, pdf and canola */
	for (i = 0; i < 40; i++) {
		for (j = 0; j < 40; j++) {
			freq_browser[i][j] = 0;
			freq_canola[i][j] = 0;
			freq_pdf[i][j] = 0;
		}
	}

	/* Save trained som in grids */
	read_trained_som("saved_som.txt", grids);

	read_freq_log("freq-pdf.log");
	read_freq_log("freq-browser.log");
	read_freq_log("freq-canola.log");

	magic();

	//read_freq_log("freq-pdf.log");
	/*
	for (i = 0; i < 40; i++) {
		for (j = 0; j < 40; j++) {
			printf("%d ", freq_pdf[i][j]);
		}
		printf("\n");
	}

	i = 0;
	do {
		mem_free_array[i] = get_memfree();
		sleep(timeout);
		i++;
	} while(i<MAX_INPUTS); */

	return 0;
}
