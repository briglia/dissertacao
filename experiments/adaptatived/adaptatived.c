#include <glib.h>
#include <unistd.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#include "som.h"
#include "util.h"
#include "ui.h"

/* Files directories */
#define NO_CC_DIR "no-cc/"
#define WITH_CC_DIR "with-cc/"
#define CC_TUN_DIR "cc-tun/"

/* This variable saves the latest dir */
char *current_dir;

/* Memory behavior profiles:
 * These profiles are the same presented in dissertation
 * text. ALPHA: CC is off. BETA: CC is on with 10MB of size. GAMA: CC is on
 * and swappiness = 60 and min_free_kbytes = 2048
 */
#define ALPHA_PROFILE	1
#define BETA_PROFILE	2
#define GAMA_PROFILE	3

/* This variable saves the latest configured profile */
int current_profile = 0;
int previous_profile = 0;

/* Boundaries for get_bmu_xy(). These values are different according to
 * running profile.
 */
int mem_max = 0;
int mem_min = 0;
int veloc_max = 0;
int veloc_min = 0;
int acel_max = 0;
int acel_min = 0;

/* Max reads from /proc/meminfo */
#define MAX_INPUTS 25

/* Total memory in kilobytes */
#define MEM 131072

/* Minimal free kilobytes before turn-off CC */
#define MIN_FREE_KB 2048

/* Array used for memFree reads */
int mem_free_array[MAX_INPUTS];

extern struct som_node * get_bmu_xy(struct som_node* [][GRIDS_YSIZE],
		unsigned int*,
		unsigned int*,
		int*, int*, int, int, int, int, int, int);

/* freq tables */
int freq_browser[GRIDS_XSIZE][GRIDS_YSIZE];
int freq_canola[GRIDS_XSIZE][GRIDS_YSIZE];
int freq_pdf[GRIDS_XSIZE][GRIDS_YSIZE];

/* Initialize freq_browser, freq_canola and freq_pdf matrix */
void init_freq_tables() {
	int i, j;
	for (i = 0; i < 40; i++) {
		for (j = 0; j < 40; j++) {
			freq_browser[i][j] = 0;
			freq_canola[i][j] = 0;
			freq_pdf[i][j] = 0;
		}
	}
}

/* Read memory, velocity and acceleration values from file. These values
 * varies according to profile used.
 */
void read_max_min_values(char filename[]) {
	FILE *fp;
	char buffer[2048];
	ssize_t bytes_read;
	size_t len = 0;
	char * line = NULL;

	fp = fopen(filename, "r");

	if (fp == NULL) {
		printf("Error opening freq log file\n");
		return;
	}

	while((bytes_read = getline(&line, &len, fp)) != -1) {
		sscanf(line, "%d %d %d %d %d %d", &mem_max, &mem_min,
				&veloc_max, &veloc_min, &acel_max, &acel_min);
	}

	if (line)
		free(line);
	if (fclose(fp))
		printf("Error closing file\n");

}

/* This method parses /proc/meminfo file and returns memfree - MEM
 */
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

/* Populates mem_free_array with values read from /proc/meminfo */
void populate_mem_free_array() {
	int i = 0;
	int timeout = 1;

	do {
		mem_free_array[i] = get_memfree();
		sleep(timeout);
		i++;
	} while(i<MAX_INPUTS);

}

/* Reads freq-{pdf,browser,canola}.log files and saves into a 40x40 matrix */
void read_freq_log(char filename[], int freq_table[GRIDS_XSIZE][GRIDS_YSIZE]) {
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
		freq_table[i][j] = freq;
	}

	if (line)
		free(line);
	if (fclose(fp))
		printf("Error closing file\n");
}

/* Reads mem_free_array and returns a rss_list struct for get_bmu_xy() */
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

/* Finds the factor (pdf, browser or canola) according to mem_free_array
 * input, trained neural network som and frequency tables */
int calculate_profile() {
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
				&accel,
				mem_max, mem_min,
				veloc_max, veloc_min,
				acel_max, acel_min);
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

	if ((pdf_factor > browser_factor) && (pdf_factor >
				canola_factor)) {
		return ALPHA_PROFILE;
	} else if (browser_factor > canola_factor)
		return GAMA_PROFILE;
	else
		return BETA_PROFILE;
}

int main()
{
	/* Starts profile as ALPHA_PROFILE: CC is not configured */
	current_profile = ALPHA_PROFILE;

	do {
		if (current_profile != previous_profile) {
			if (current_profile == ALPHA_PROFILE) {
				printf("ALPHA PROFILE\n");
				system("sh unuse_compcache.sh");

				previous_profile = ALPHA_PROFILE;

				/* Save trained som in grids struct */
				read_trained_som(NO_CC_DIR"saved_som.txt", grids);

				init_freq_tables();

				read_freq_log(NO_CC_DIR"freq-pdf.log", freq_pdf);
				read_freq_log(NO_CC_DIR"freq-browser.log", freq_browser);
				read_freq_log(NO_CC_DIR"freq-canola.log", freq_canola);

				read_max_min_values(NO_CC_DIR"max_min_values.txt");
			} else if (current_profile == BETA_PROFILE) {
				printf("BETA PROFILE\n");
				system("sh unuse_compcache.sh");
				system("sh use_compcache.sh 5120");

				previous_profile = BETA_PROFILE;

				/* Save trained som in grids struct */
				read_trained_som(WITH_CC_DIR"saved_som.txt", grids);

				init_freq_tables();

				read_freq_log(WITH_CC_DIR"freq-pdf.log", freq_pdf);
				read_freq_log(WITH_CC_DIR"freq-browser.log", freq_browser);
				read_freq_log(WITH_CC_DIR"freq-canola.log", freq_canola);

				read_max_min_values(WITH_CC_DIR"max_min_values.txt");
			} else if (current_profile == GAMA_PROFILE) {
				printf("GAMA PROFILE\n");
				system("sh unuse_compcache.sh");
				system("sh use_compcache.sh 10240");
				system("echo 60 > /proc/sys/vm/swappiness");

				previous_profile = GAMA_PROFILE;

				/* Save trained som in grids struct */
				read_trained_som(WITH_CC_DIR"saved_som.txt", grids);

				init_freq_tables();

				read_freq_log(WITH_CC_DIR"freq-pdf.log", freq_pdf);
				read_freq_log(WITH_CC_DIR"freq-browser.log", freq_browser);
				read_freq_log(WITH_CC_DIR"freq-canola.log", freq_canola);

				read_max_min_values(WITH_CC_DIR"max_min_values.txt");

			}
		}

		current_profile = calculate_profile();
	} while(1);

	return 0;
}
