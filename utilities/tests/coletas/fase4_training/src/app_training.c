#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "som.h"

extern int read_som_input(struct input_pattern* [], int);
extern void init_grid(struct som_node* [][GRIDS_YSIZE]);
extern void train(struct input_pattern* [], int);
extern void save_trained_som(char*, struct som_node* [][GRIDS_YSIZE]);


GtkWidget *drawingarea[GRIDS_XSIZE][GRIDS_YSIZE];
GdkColor color;


/* 
 * Close down and exit handler 
 */
gint destroy_window(GtkWidget * widget,
                    GdkEvent  * event,
                    gpointer    client_data) {
	gtk_main_quit();
	return TRUE;
}
		    

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
			
			color.red = (int)(grids[i][j]->weights[0] * 65535);
			color.blue = (int)(grids[i][j]->weights[1] * 65535);
			color.green = (int)(grids[i][j]->weights[2] * 65535);
			
			gtk_widget_modify_bg(drawingarea[i][j], GTK_STATE_NORMAL, &color);
			
			/* Set size of drawingarea */
			gtk_widget_set_size_request(GTK_WIDGET(drawingarea[i][j]), 10, 10);
			
			/* Insert drawing area into the upper left quadrant of the table */
			gtk_table_attach_defaults(GTK_TABLE(table), drawingarea[i][j], i, i+1, j, j+1);

			gtk_widget_show(drawingarea[i][j]);
		}
	}

	return table;
}


void repaint_gtk_table(GtkWidget * widget, gpointer data) {
	int i, j;
	int input_size;

	input_size = read_som_input(inputs, INPUT_MAXSIZE);
	
	train(inputs, input_size);
	
	save_trained_som("saved_som.txt", grids);

	for (i=0; i<GRIDS_XSIZE; i++) {
		for (j=0; j<GRIDS_YSIZE; j++) {
			color.red = (int)(grids[i][j]->weights[0] * 65535);
			color.blue = (int)(grids[i][j]->weights[1] * 65535);
			color.green = (int)(grids[i][j]->weights[2] * 65535);
			gtk_widget_modify_bg(drawingarea[i][j], GTK_STATE_NORMAL, &color);
		}
	}
}


gint main(gint argc, gchar * argv[]) {
	GtkWidget *window;
	GtkWidget *tablebox;
	GtkWidget *table;
	GtkWidget *separator;
	GtkWidget *mainbox;
	GtkWidget *buttonbox;
	GtkWidget *button;
	
	/* Initiate grids */
	init_grid(grids);	

	/* Initialize the toolkit, remove gtk-related commandline stuff */
	gtk_init(&argc, &argv);
	
	/* Create toplevel window, set title and policies */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "SOM - Self Organizing Maps");
	gtk_container_set_border_width (GTK_CONTAINER (window), 4);

	/* Attach to the "delete" and "destroy" events so we can exit */
	g_signal_connect(GTK_OBJECT(window), "delete_event",
			 GTK_SIGNAL_FUNC(destroy_window), (gpointer)window);
  
	mainbox = gtk_vbox_new(FALSE, 5);

	gtk_container_add(GTK_CONTAINER(window), mainbox);

	tablebox = gtk_vbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(mainbox), tablebox, TRUE, FALSE, 0);
	
	/* Create and initiate gtk table*/
	table = create_gtk_table(table, mainbox);

	gtk_widget_show(table);

	gtk_widget_show(tablebox);

	separator = gtk_hseparator_new ();

	gtk_widget_set_size_request (separator, 400, 5);

	gtk_box_pack_start(GTK_BOX (mainbox), separator, TRUE, FALSE, 0);

	gtk_widget_show(separator);

	buttonbox = gtk_hbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(mainbox), buttonbox, TRUE, FALSE, 0);

	button = gtk_button_new_with_label("Start Training");

	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(repaint_gtk_table), NULL);

	gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, FALSE, 0);

	gtk_widget_show(button);

	gtk_widget_show(buttonbox);

	gtk_widget_show(mainbox);

	gtk_widget_show(window);

	/* Enter the gtk main loop (this never returns) */	
	gtk_main();
	
	return 0;
}
