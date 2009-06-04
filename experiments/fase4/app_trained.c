#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <unistd.h>
#include "som.h"
#include "util.h"

#define MN_OPEN_DIALOG 0
#define MN_SAVE_DIALOG 1

extern void read_trained_som(char*, struct som_node* [][GRIDS_YSIZE]);
extern int read_som_input(struct input_pattern* [], int);
extern void train(struct input_pattern* [], int);
extern void save_trained_som(char*, struct som_node* [][GRIDS_YSIZE]);
extern int get_procs(struct proc***);
extern void free_procs(struct proc***, int);
extern struct som_node * get_bmu_xy(struct som_node* [][GRIDS_YSIZE],
				    unsigned int*,
				    unsigned int*,
				    int*, int*);

public GtkWidget *drawingarea[GRIDS_XSIZE][GRIDS_YSIZE];
GdkColor color;
GtkWidget *combobox;
GtkWidget *refresh_button;
GtkWidget *label;
GtkWidget *entry;
GtkWidget *rbruntime;

gchar *dirname;

struct proc ** proclist;
int length;
int this_pid;


/* 
 * Close down and exit handler 
 */
gint destroy_window(GtkWidget * widget,
                    GdkEvent  * event,
                    gpointer    data) {
	gtk_main_quit();
	return TRUE;
}


/*
 * Print the data of a drawing area when clicked
 */
gboolean button_press(GtkWidget *widget,
		      GdkEventExpose *event,
		      gpointer data) {
	int x, y, z;
	char mem[10];
	char veloc[10];
	char accel[10];
	struct som_node * node = (struct som_node *)data;
	x = undo_normalize(node->weights[0], MEM_MAX, MEM_MIN);
	y = undo_normalize(node->weights[1], VELOC_MAX, VELOC_MIN);
	z = undo_normalize(node->weights[2], ACEL_MAX, ACEL_MIN);
	
	printf("(%d, %d) [%d %d %d] [%s, %s, %s]\n",
	       node->xp, node->yp,
	       x, y, z,
	       get_classname(x, MEM_MAX, MEM_MIN, mem),
	       get_classname(y, VELOC_MAX, VELOC_MIN, veloc),
	       get_classname(z, ACEL_MAX, ACEL_MIN, accel)
		);
	
	return TRUE;
}


/*
 * Build a filtering for the file chooser dialogs:
 * gchar **mime: it is a set of patterns e.g: {"test.*","*.html",NULL}
 */
GtkFileFilter* create_filter(gchar **mime)
{
        GtkFileFilter *filter= gtk_file_filter_new ();

        while (*mime)
        {
                gtk_file_filter_add_pattern (filter, *mime);
                *mime++;
        }
	return filter;
} 


/* 
 * Create a file chooser window with filters
 * title: the title of the file chooser dialog.
 * action: the type of the dialog: open or save dialog
 * mime: it is a set of patterns used in the filters e.g: {"teste.*","*.html",NULL}
 * fileName: a standard name for the file to be save
 * parentWindow: the parent window of the dialog
 */
gchar* create_path_selection(gchar *title,
			     gint action,
			     gchar **mime,
			     gchar *fileName,
			     GtkWidget *parentWindow) {
        char *filename;
        GtkWidget *dialog;
        GtkFileFilter *chooser_filter;
	
        switch (action) {
        case MN_OPEN_DIALOG:
                dialog = gtk_file_chooser_dialog_new(title,
						     GTK_WINDOW(parentWindow),
						     GTK_FILE_CHOOSER_ACTION_OPEN,
						     GTK_STOCK_CANCEL,
						     GTK_RESPONSE_CANCEL,
						     GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						     NULL);
                break;
        case MN_SAVE_DIALOG:
                dialog = gtk_file_chooser_dialog_new(title,
						     GTK_WINDOW(parentWindow),
						     GTK_FILE_CHOOSER_ACTION_SAVE,
						     GTK_STOCK_CANCEL,
						     GTK_RESPONSE_CANCEL,
						     GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						     NULL);
                break;
        default:
                break;
        }
	
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), g_get_current_dir());
        
	if (fileName)
                gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), fileName);
        
	chooser_filter = create_filter(mime);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), chooser_filter);
	
        if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
                filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        }
        else {
		filename = NULL;
	}
	
        gtk_widget_destroy(dialog);
	
        return filename;
}


/*
 * Callback for radio button "toggled" signal
 */
void radio_button_callback(GtkWidget *widget, gint data) {
	int i;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
		/* Radio button rbruntime selected */
		if (data == 0) {
			char item[40];
			
			/* Get the list of applications */
			length = get_procs(&proclist);
			
			/* Add one by one in the combobox */
			for (i=0; i<length; i++) {
				if (this_pid != proclist[i]->pid) {
					sprintf(item, "(%u) - %s",
						proclist[i]->pid,
						proclist[i]->cmd);
					gtk_combo_box_append_text(
						GTK_COMBO_BOX(combobox),
						item);
				}
			}
			
			/* Select first item as default */
			gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);

			/* Show the widgets */
			gtk_widget_show(label);
			gtk_widget_show(entry);
			gtk_widget_show(refresh_button);
		}
		/* Radio button rblog selected */
		else {
			/* Filter for open dialog window*/
			gchar *mime[] = {"*.log", NULL};

			const gchar *logname;
			const gchar *file;
			GDir * directory;

			/* Return the filename by open dialog window */
			gchar *filename = create_path_selection("Open log file",
								MN_OPEN_DIALOG,
								mime,
								NULL,
								NULL);

			/* If filename was chosen in the open dialog window */
			if (filename != NULL) {
				/* Get the path name */
				dirname = g_path_get_dirname(filename);

				/* Get just the file name */
				file = g_path_get_basename(filename);
				
				/* Open the directory for reading */
				directory = g_dir_open(dirname, 0, NULL);

				length = 0;
				
				/* Go through the directory */
				while ((logname = g_dir_read_name(directory))) {

					/* Select file with ".log" suffix */
					if (g_str_has_suffix(logname, ".log")) {
						gtk_combo_box_append_text(
							GTK_COMBO_BOX(combobox),
							logname);
						if (!g_ascii_strcasecmp(logname, file)) {
							gtk_combo_box_set_active(
								GTK_COMBO_BOX(combobox), 
								length);
						}
						length++;
					}
				}

				/* Close the directory */
				g_dir_close(directory);

				/* Hide the widgets */
				gtk_widget_hide(label);
				gtk_widget_hide(entry);
				gtk_widget_hide(refresh_button);
			}
			/* Otherwise select the radio button rbruntime*/
			else {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rbruntime),
							     TRUE);
			}
		}
	}
	else {
		if (data == 0) {
			/* Remove all itens*/
			for (i=0; i<length; i++) 
				gtk_combo_box_remove_text(GTK_COMBO_BOX(combobox), 0);
			
			/* Free the list of applications */
			free_procs(&proclist, length);
		}
		else {
			/* Remove all itens*/
			for (i=0; i<length; i++) 
				gtk_combo_box_remove_text(GTK_COMBO_BOX(combobox), 0);
		}
	}
}


/*
 * Reset the gtk table
 */
void reset_gtk_table(GtkWidget * table,
		     GtkWidget * box) {
	int i, j;

	for (i=0; i<GRIDS_XSIZE; i++) {
		for (j=0; j<GRIDS_YSIZE; j++) {

			color.red = (int)(grids[i][j]->weights[0] * 65535);
			color.blue = (int)(grids[i][j]->weights[1] * 65535);
			color.green = (int)(grids[i][j]->weights[2] * 65535);
			
			/* Modify the bg color of drawing area */
			gtk_widget_modify_bg(drawingarea[i][j],
					     GTK_STATE_NORMAL,
					     &color);
		}
	}
}


/*
 * Create the gtk table
 */
GtkWidget * create_gtk_table(GtkWidget * table,
			     GtkWidget * box) {
	int i, j;

	table = gtk_table_new(GRIDS_YSIZE, GRIDS_XSIZE, TRUE);

	/* Put the table in the main window */
	gtk_box_pack_start(GTK_BOX(box), table, TRUE, FALSE, 0);

	for (i=0; i<GRIDS_XSIZE; i++) {
		for (j=0; j<GRIDS_YSIZE; j++) {
			/* Create drawingarea */
			drawingarea[i][j] = gtk_drawing_area_new();
			
			gtk_widget_add_events(drawingarea[i][j],
					      GDK_BUTTON_PRESS_MASK);
			
			g_signal_connect(G_OBJECT(drawingarea[i][j]),
					 "button_press_event",  
					 G_CALLBACK(button_press),
					 (gpointer)grids[i][j]);

			color.red = (int)(grids[i][j]->weights[0] * 65535);
			color.blue = (int)(grids[i][j]->weights[1] * 65535);
			color.green = (int)(grids[i][j]->weights[2] * 65535);
			
			/* Modify the bg color of drawing area */
			gtk_widget_modify_bg(drawingarea[i][j],
					     GTK_STATE_NORMAL,
					     &color);
			
			/* Set size of drawingarea */
			gtk_widget_set_size_request(
				GTK_WIDGET(drawingarea[i][j]),
				10, 10);
			
			/* 
			 * Insert drawing area into the upper left quadrant
			 * of the table 
			 */
			gtk_table_attach_defaults(GTK_TABLE(table),
						  drawingarea[i][j],
						  i, i+1, j, j+1);

			gtk_widget_show(drawingarea[i][j]);
		}
	}

	return table;
}


/*
 * Create the gtk combo box
 */
GtkWidget * create_gtk_combo_box(GtkWidget * combobox,
				 GtkWidget * box) {
	int i;
	char item[40];

	/* Create combobox */
	combobox = gtk_combo_box_new_text();

	gtk_box_pack_start(GTK_BOX(box), combobox, TRUE, FALSE, 0);
	
	/* Get the list of applications */
	length = get_procs(&proclist);

	/* Add one by one in the combobox */
	for (i=0; i<length; i++) {
		if (this_pid != proclist[i]->pid) {
			sprintf(item, "(%u) - %s",
				proclist[i]->pid,
				proclist[i]->cmd);
			gtk_combo_box_append_text(GTK_COMBO_BOX(combobox),
						  item);
		}
	}

	return combobox;
}


/*
 * Update the gtk combo box
 */
void update_gtk_combo_box(GtkWidget * widget, GtkWidget * combobox) {
	int i;
	char item[40];

	/* Remove all itens*/
	for (i=0; i<length; i++) 
		gtk_combo_box_remove_text(GTK_COMBO_BOX(combobox), 0);

	/* Free the list of applications */
	free_procs(&proclist, length);

	/* Get the list of applications */
	length = get_procs(&proclist);

	/* Add one by one in the combobox */
	for (i=0; i<length; i++) {
		if (this_pid != proclist[i]->pid) {
			sprintf(item, "(%u) - %s",
				proclist[i]->pid,
				proclist[i]->cmd);
			gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), item);
		}
	}

	/* Select first item as default */
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
}


/*
 * Monitor the memory consumption pattern of application
 * in the trained SOM
 */
void run(GtkWidget * widget, GtkWidget * combobox) {
	pid_t pid;
	unsigned int current_rss = 0;
	unsigned int new_rss;
	int veloc = 0;
	int accel = 0;
	struct som_node * bmu = NULL;
	int i, j;
	unsigned long x;
	unsigned long iterations;
	char filename[30];
	gchar *fullname;

	struct rss_list * head = NULL;
	struct rss_list * iterator = NULL;

	/* Runtime radion button is active */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rbruntime))) {
		/* Get the iteration value from text entry */
		iterations = atol(gtk_entry_get_text(GTK_ENTRY(entry)));
		
		/* Get the pid number from combo box */
		sscanf(gtk_combo_box_get_active_text((GTK_COMBO_BOX(combobox))),
		       "(%u)",
		       &pid);
	}
	
	/* Logfile radion button is active */
	else {
		gsize size;
		/* Get the log filename from combo box */
		sscanf(gtk_combo_box_get_active_text((GTK_COMBO_BOX(combobox))),
		       "%s",
		       filename);
		/* Calculate the size of memory for fullname string */
		size = g_string_new(dirname)->len + g_string_new(filename)->len + 2;
		
		/* Allocate memory */
		fullname = (gchar *)malloc(size);
		
		/* Copy the dirname contents to fullname*/
		g_stpcpy(fullname, dirname);

		/* Append "/" at end of fullname*/
		sprintf(fullname, "%s/", fullname);
		
		/* Concatenate the string */
		g_strlcat(fullname, filename, size);
		
		/* Get the head pointer for rss_list */
		head = read_log_statm(fullname, &iterations);
		iterator = head;
		
		/* Free memory */
		free(fullname);
	}

	/*
	int dx, dy, dz;
	char mem_flag[10];
	char veloc_flag[10];
	char accel_flag[10];
	*/

	for (x=0; x<iterations; x++) {
		/* Runtime radion button is active */
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rbruntime))) {
			/* Get the rss of pid */
			new_rss = read_statm(pid);
		}

		/* Logfile radion button is active */
		else {
			new_rss = iterator->rss_pages;
			iterator = iterator->next;
		}
		
		/* Get the closest bmu */
		bmu = get_bmu_xy(grids,
				 &new_rss,
				 &current_rss,
				 &veloc,
				 &accel);
		
		/*
		dx = undo_normalize(bmu->weights[0], MEM_MAX, MEM_MIN);
		dy = undo_normalize(bmu->weights[1], VELOC_MAX, VELOC_MIN);
		dz = undo_normalize(bmu->weights[2], ACEL_MAX, ACEL_MIN);
		*/

		/* Just for debugging purposes */

		/*
		printf("(%d, %d) [%d %d %d] [%s, %s, %s]\n",
		       bmu->xp, bmu->yp,
		       dx, dy, dz,
		       get_classname(dx, MEM_MAX, MEM_MIN, mem_flag),
		       get_classname(dy, VELOC_MAX, VELOC_MIN, veloc_flag),
		       get_classname(dz, ACEL_MAX, ACEL_MIN, accel_flag)
			);
		*/

		i = bmu->xp;
		j = bmu->yp;
		
		/* Set white color */
		color.red = (int)65535;
		color.blue = (int)65535;
		color.green = (int)65535;
		
		/* Paint the area with white color */
		gtk_widget_modify_bg(drawingarea[i][j],
				     GTK_STATE_NORMAL,
				     &color);
		
		/* Run all pending events and high priority idle functions */
		while(g_main_context_iteration(NULL, FALSE));
		
		/* Repaint the same are with original color */
		color.red = (int)(grids[i][j]->weights[0] * 65535);
		color.blue = (int)(grids[i][j]->weights[1] * 65535);
		color.green = (int)(grids[i][j]->weights[2] * 65535);
		
		/* Paint the area with original color */
		/*
		gtk_widget_modify_bg(drawingarea[i][j],
				     GTK_STATE_NORMAL,
				     &color);
		*/
		//usleep(10);
	}
	
	if (head)
		free_rss_list(head);

	g_print("\nExecution Terminated.\n");
}


gint main(gint argc, gchar * argv[]) {
	GtkWidget *window;
	GtkWidget *mainframe;
	GtkWidget *tablebox;
	GtkWidget *table;
	GtkWidget *separator;
	GtkWidget *mainbox;
	GtkWidget *topbox;
	GtkWidget *run_button;
	GtkWidget *bottombox;
	GtkWidget *rblog;
	GtkWidget *labelentrybox;
	GtkWidget *reset_button;

	this_pid = (int)getpid();
	
	/* Initiate grids */	
	read_trained_som("saved_som.txt", grids);

	/* Initialize the toolkit, remove gtk-related commandline stuff */
	gtk_init(&argc, &argv);
	
	/* Create toplevel window, set title and policies */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "SOM - Self Organizing Maps");
	gtk_container_set_border_width (GTK_CONTAINER (window), 4);

	/* Attach to the "delete" and "destroy" events so we can exit */
	/*g_signal_connect(GTK_OBJECT(window), "delete_event",
			 GTK_SIGNAL_FUNC(destroy_window), (gpointer)window);*/

	mainbox = gtk_vbox_new(FALSE, 5);

	gtk_container_add(GTK_CONTAINER(window), mainbox);

	tablebox = gtk_vbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(mainbox), tablebox, TRUE, FALSE, 0);
	
	/* Create and initiate gtk table */
	table = create_gtk_table(table, mainbox);

	gtk_widget_show(table);

	gtk_widget_show(tablebox);
	
	mainframe = gtk_frame_new("Parameters Tuning");
	
	gtk_box_pack_start(GTK_BOX(mainbox), mainframe, TRUE, FALSE, 0);
	
	topbox = gtk_hbox_new(FALSE, 0);
	
	gtk_container_set_border_width(GTK_CONTAINER(topbox), 5);
	
	gtk_container_add(GTK_CONTAINER(mainframe), topbox);
	
	/* Create and initiate gtk combobox */
	combobox = create_gtk_combo_box(combobox, topbox);
	
	/* Select first item as default */
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
	
	/* Radio button for runtime selection */
	rbruntime = gtk_radio_button_new_with_label(NULL, "Runtime");
	g_signal_connect(G_OBJECT(rbruntime), "toggled",
			 G_CALLBACK(radio_button_callback), (gpointer)0);
	gtk_box_pack_start(GTK_BOX(topbox), rbruntime, TRUE, FALSE, 0);
	gtk_widget_show(rbruntime);
	
	/* Radio button for log mode selection */
	rblog = gtk_radio_button_new_with_label_from_widget
		(GTK_RADIO_BUTTON(rbruntime), "Logfile");
	g_signal_connect(G_OBJECT(rblog), "toggled",
			 G_CALLBACK(radio_button_callback), (gpointer)1);
	gtk_box_pack_start(GTK_BOX(topbox), rblog, TRUE, FALSE, 0);
	gtk_widget_show(rblog);

	labelentrybox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new ("Time: ");
	gtk_box_pack_start(GTK_BOX(labelentrybox), label, TRUE, FALSE, 0);
	gtk_widget_show(label);

	/* Text entry for informing the number of iterations */
	entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 6);
	gtk_entry_set_text(GTK_ENTRY(entry), "4000");
	gtk_entry_set_width_chars(GTK_ENTRY(entry), 6);
	gtk_box_pack_start(GTK_BOX(labelentrybox), entry, TRUE, FALSE, 0);
	gtk_widget_show(entry);

	gtk_box_pack_start(GTK_BOX(topbox), labelentrybox, TRUE, FALSE, 0);
	gtk_widget_show(labelentrybox);

	bottombox = gtk_hbox_new(FALSE, 0);
	
	gtk_container_set_border_width(GTK_CONTAINER(bottombox), 2);

	gtk_box_pack_start(GTK_BOX(mainbox), bottombox, TRUE, FALSE, 0);

	/* Create running button */
	run_button = gtk_button_new_with_label("Start Running");

	g_signal_connect(G_OBJECT(run_button), "clicked",
			 G_CALLBACK(run), combobox);
	
	gtk_box_pack_start(GTK_BOX(bottombox), run_button, TRUE, FALSE, 0);

	/* Create refresh button */
	refresh_button = gtk_button_new_with_label("Refresh");

	gtk_box_pack_start(GTK_BOX(bottombox), refresh_button, TRUE, FALSE, 0);

	g_signal_connect(G_OBJECT(refresh_button), "clicked",
			 G_CALLBACK(update_gtk_combo_box), combobox);
	
	/* Create reset button */
	reset_button = gtk_button_new_with_label("Reset");

	gtk_box_pack_start(GTK_BOX(bottombox), reset_button, TRUE, FALSE, 0);
	
	g_signal_connect(G_OBJECT(reset_button), "clicked",
			 G_CALLBACK(reset_gtk_table), combobox);

	separator = gtk_hseparator_new();

	gtk_widget_set_size_request(separator, 400, 5);

	gtk_box_pack_start(GTK_BOX(mainbox), separator, TRUE, FALSE, 0);

	gtk_widget_show(separator);

	gtk_widget_show(combobox);

	gtk_widget_show(run_button);

	gtk_widget_show(reset_button);
	
	gtk_widget_show(refresh_button);

	gtk_widget_show(topbox);

	gtk_widget_show(bottombox);

	gtk_widget_show(mainframe);

	gtk_widget_show(mainbox);

	gtk_widget_show(window);

	/* Enter the gtk main loop (this never returns) */	
	gtk_main();

	return 0;
}
