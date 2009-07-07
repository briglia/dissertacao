#include <glib.h>
#include <unistd.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "som.h"
#include "ui.h"
#include "util.h"

extern int read_som_input(struct input_pattern* [], int);
int read_trained_som(char filename[], struct som_node * grids[][GRIDS_YSIZE]); 
extern void init_grid(struct som_node* [][GRIDS_YSIZE]);
extern void train(struct input_pattern* [], int);
extern void save_trained_som(char*, struct som_node* [][GRIDS_YSIZE]);
extern struct som_node * get_bmu_xy(struct som_node* [][GRIDS_YSIZE],
		unsigned int*,
		unsigned int*,
		int*, int*);
void reset_percentual(void); 
void update_percentualbox(void);

#define MN_OPEN_DIALOG 0
#define MN_SAVE_DIALOG 1



/*
 * Status bar - pushing item 
 * */
void status_bar_push_item(gchar *info)
{
	gtk_statusbar_push (GTK_STATUSBAR (status_bar), GPOINTER_TO_INT (context_id),info);
}






/*
 * Monitor the memory consumption pattern of application
 * in the trained SOM
 */
void run(void) {
	reset_percentual();
	stop=0; 
	GdkColormap *colormap; 
	unsigned int current_rss = 0;
	unsigned int new_rss;
	int veloc = 0;
	int accel = 0;
	struct som_node * bmu = NULL;
	int i, j;
	unsigned long x;
	unsigned long iterations;
	gchar *fullname;

	struct rss_list * head = NULL;
	struct rss_list * iterator = NULL;

	fullname=filename; 

	/* Get the head pointer for rss_list */
	head = read_log_statm(fullname, &iterations);
	iterator = head;

	/* Free memory */
	free(fullname);

	
	int dx, dy, dz;
	/*
	char mem_flag[10];
	char veloc_flag[10];
	char accel_flag[10];
	*/

	for (x=0; x<iterations; x++) {
		

		/* Logfile radion button is active */
		new_rss = iterator->rss_pages;
		iterator = iterator->next;

		/* Get the closest bmu */
		bmu = get_bmu_xy(grids,
				 &new_rss,
				 &current_rss,
				 &veloc,
				 &accel);
		
		/*Percentual update*/	
		dx = undo_normalize(bmu->weights[0], MEM_MAX, MEM_MIN);
		dy = undo_normalize(bmu->weights[1], VELOC_MAX, VELOC_MIN);
		dz = undo_normalize(bmu->weights[2], ACEL_MAX, ACEL_MIN);
		car_con[get_classposition(dx, MEM_MAX, MEM_MIN)-3]++; 
		car_var[get_classposition(dy, VELOC_MAX, VELOC_MIN)]++; 
		car_tax[get_classposition(dz, ACEL_MAX, ACEL_MIN)]++;


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
		while(g_main_context_iteration(NULL, FALSE));
		usleep(1000);

		
		/* Set black color */
		color.red = (int)0;
		color.blue = (int)0;
		color.green = (int)0;
		/* Paint the area with black color */
		gtk_widget_modify_bg(drawingarea[i][j],
				     GTK_STATE_NORMAL,
				     &color);
		/* Run all pending events and high priority idle functions */
		while(g_main_context_iteration(NULL, FALSE));
		usleep(1000);


		/*Freq table increment -- color decrement*/
		colormap=gtk_widget_get_colormap(drawingfreq[i][j]); 
		color=drawingfreq[i][j]->style->bg[0]; 
		if(color.red>100){
			color.red=color.red-100;; 	
			color.green=color.green-100; 
			color.blue=color.blue-100;
		}
		/* Paint the area on freq  */
		gtk_widget_modify_bg(drawingfreq[i][j],
				     GTK_STATE_NORMAL,
				     &color);
		while(g_main_context_iteration(NULL, FALSE));
		usleep(1000);



		/* Repaint the same are with original color */
		color.red = (int)(grids[i][j]->weights[0] * 65535);
		color.blue = (int)(grids[i][j]->weights[1] * 65535);
		color.green = (int)(grids[i][j]->weights[2] * 65535);
		/* Paint the area with original color */
		gtk_widget_modify_bg(drawingarea[i][j],
				     GTK_STATE_NORMAL,
				     &color);

		/* Paint the sele area with original color */
		gtk_widget_modify_bg(drawingsele[i][j],
				     GTK_STATE_NORMAL,
				     &color);

		while(g_main_context_iteration(NULL, FALSE));
		usleep(1000);


		update_percentualbox();


		if(stop){
			stop=0; 
			break; 
			} 


	}
	
	if (head)
		free_rss_list(head);
	status_bar_push_item(g_strdup_printf("Executation terminated : %s",filename));
}


/*
 *  * Build a filtering for the file chooser dialogs:
 *   * gchar **mime: it is a set of patterns e.g: {"test.*","*.html",NULL}
 *    */
GtkFileFilter* create_filter(gchar **mime)
{
	GtkFileFilter *filter= gtk_file_filter_new ();

	while (*mime)
	{
		gtk_file_filter_add_pattern (filter, *mime++);
		//*mime++;
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
 * Select file 
 */
void select_log_file(GtkWidget *widget, gint data) {

	/* Radio button rblog selected */
	/* Filter for open dialog window*/
	gchar *mime[] = {"*.log", NULL};


	/* Return the filename by open dialog window */
	filename = create_path_selection("Open log file",
			MN_OPEN_DIALOG,
			mime,
			NULL,
			NULL);

	/* If filename was chosen in the open dialog window */
	if (filename != NULL) {
		status_bar_push_item(g_strdup_printf("RUNNING : %s",filename));	
		run();
	}
	else {
		status_bar_push_item(g_strdup_printf("WARNING : Select a log file "));
	}

}


/*
 * Stop log file executation
 */
void stop_logfile(GtkWidget *widget, gint data) {
	stop=1; 
}




/*
 *  * Print the data of a drawing area when clicked
 *   */
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
	status_bar_push_item(g_strdup_printf("(%d, %d) [%d %d %d] [%s, %s, %s]",
			node->xp, node->yp,
			x, y, z,
			get_classname(x, MEM_MAX, MEM_MIN, mem),
			get_classname(y, VELOC_MAX, VELOC_MIN, veloc),
			get_classname(z, ACEL_MAX, ACEL_MIN, accel)
	      )
	      );

	return TRUE;
}



/*
 * Reset percentual  
 */
void reset_percentual(void) {

	int i;
	for (i=0; i<3; i++) {
		car_con[i]=0; 

	}
	for (i=0; i<6; i++) {
		car_var[i]=0; 
		car_tax[i]=0; 

	}
}

/*
 * Generate trained percentual   
 */
void calculate_trained_percentual(void) {

	int x, y, z,i,j;
	for (i=0; i<GRIDS_XSIZE; i++) {
		for (j=0; j<GRIDS_YSIZE; j++) {
			x = undo_normalize(grids[i][j]->weights[0], MEM_MAX, MEM_MIN);
			y = undo_normalize(grids[i][j]->weights[1], VELOC_MAX, VELOC_MIN);
			z = undo_normalize(grids[i][j]->weights[2], ACEL_MAX, ACEL_MIN);
			car_con[get_classposition(x, MEM_MAX, MEM_MIN)-3]++; 
			car_var[get_classposition(y, VELOC_MAX, VELOC_MIN)]++; 
			car_tax[get_classposition(z, ACEL_MAX, ACEL_MIN)]++;
			//printf("pos : %d\n",get_classposition(x, MEM_MAX, MEM_MIN)-3) ; 

		}
	}


}

/*
 * Update percentual box : update all progress bars on percentual box area using cararc vectors 
 */
void update_percentualbox(void){
	int i;
	unsigned long acu=0;
	for (i=0; i<3; i++) {
		acu=acu+car_con[i]; 
		//printf("pos %d val %ld \n",i,car_con[i]); 
	}
	for (i=0; i<3; i++) {
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(con[i]),((double)car_con[i]/acu)) ; 
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR(con[i]),g_strdup_printf("%f %%",100*((double)car_con[i]/acu)));
		printf ("size [%d]=%f %% ",i,100*((double)car_con[i]/acu)); 
	}
	while(g_main_context_iteration(NULL, FALSE));
	
	for (i=0; i<6; i++) {
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(var[i]),((double)car_var[i]/acu));
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR(var[i]),g_strdup_printf("%f %%",100*((double)car_var[i]/acu)));
		printf ("%f %% ",100*((double)car_var[i]/acu));
		printf ("MUV [%d]=%f %% ",i,100*((double)car_var[i]/acu));
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(tax[i]),((double)car_tax[i]/acu));
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR(tax[i]),g_strdup_printf("%f %%",100*((double)car_tax[i]/acu)));
		printf ("%f %%",100*((double)car_tax[i]/acu)); 
		printf ("RMUV [%d]=%f %%",i,100*((double)car_tax[i]/acu)); 
	}

	while(g_main_context_iteration(NULL, FALSE));
	printf("\n");

}

/*
 * Update percentual 
 * */
void update_percentual_trained(void){
	reset_percentual(); 
	calculate_trained_percentual();
	update_percentualbox(); 	

}

/*
 * Trained percentual   
 */
void trained_percentual(GtkWidget * table,
		GtkWidget * box) {
	reset_percentual(); 
	calculate_trained_percentual();
	update_percentualbox(); 	
}

/*
 *  * Reset the gtk table
 *   */
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

			color.red = (int)65535;
			color.blue = (int)65535;
			color.green = (int)65535;

			/* Modify the bg color of drawing area */
			gtk_widget_modify_bg(drawingfreq[i][j],
					GTK_STATE_NORMAL,
					&color);

			/* Modify the bg color of drawing area */
			gtk_widget_modify_bg(drawingsele[i][j],
					GTK_STATE_NORMAL,
					&color);


		}
	}
}


/* 
 * Close down and exit handler 
 */
gint destroy_window(GtkWidget * widget,
                    GdkEvent  * event,
                    gpointer    client_data) {
	gtk_main_quit();
	return TRUE;
}
		    
/*
 * Create gtk table 
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

			/*Adding event to draw area */
			gtk_widget_add_events(drawingarea[i][j],
					GDK_BUTTON_PRESS_MASK);

			g_signal_connect(G_OBJECT(drawingarea[i][j]),
					"button_press_event",
					G_CALLBACK(button_press),
					(gpointer)grids[i][j]);

			
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

/*
 * Create freq table 
 */
GtkWidget * create_gtk_freq_table(GtkWidget * table,
			     GtkWidget * box) {
	int i, j;

	table = gtk_table_new(GRIDS_YSIZE, GRIDS_XSIZE, TRUE);

	/* Put the table in the main window */
	gtk_box_pack_start(GTK_BOX(box), table, TRUE, FALSE, 0);

	for (i=0; i<GRIDS_XSIZE; i++) {
		for (j=0; j<GRIDS_YSIZE; j++) {
			/* Create drawingarea */
			drawingfreq[i][j] = gtk_drawing_area_new();

			/*Adding event to draw area */
			gtk_widget_add_events(drawingfreq[i][j],
					GDK_BUTTON_PRESS_MASK);
			
			g_signal_connect(G_OBJECT(drawingfreq[i][j]),
					"button_press_event",
					G_CALLBACK(button_press),
					(gpointer)grids[i][j]);

			/*
			color.red = (int)(grids[i][j]->weights[0] * 65535);
			color.blue = (int)(grids[i][j]->weights[1] * 65535);
			color.green = (int)(grids[i][j]->weights[2] * 65535);
			*/
			color.red = (int)65535;
			color.blue = (int)65535;
			color.green = (int)65535;

			gtk_widget_modify_bg(drawingfreq[i][j], GTK_STATE_NORMAL, &color);
			
			/* Set size of drawingarea */
			gtk_widget_set_size_request(GTK_WIDGET(drawingfreq[i][j]), 10, 10);
			
			/* Insert drawing area into the upper left quadrant of the table */
			gtk_table_attach_defaults(GTK_TABLE(table), drawingfreq[i][j], i, i+1, j, j+1);

			gtk_widget_show(drawingfreq[i][j]);
		}
	}

	return table;
}

/*
 * Create sele table 
 */
GtkWidget * create_gtk_sele_table(GtkWidget * table,
			     GtkWidget * box) {
	int i, j;

	table = gtk_table_new(GRIDS_YSIZE, GRIDS_XSIZE, TRUE);

	/* Put the table in the main window */
	gtk_box_pack_start(GTK_BOX(box), table, TRUE, FALSE, 0);

	for (i=0; i<GRIDS_XSIZE; i++) {
		for (j=0; j<GRIDS_YSIZE; j++) {
			/* Create drawingsele */
			drawingsele[i][j] = gtk_drawing_area_new();

			/*Adding event to draw area */
			gtk_widget_add_events(drawingsele[i][j],
					GDK_BUTTON_PRESS_MASK);
			g_signal_connect(G_OBJECT(drawingsele[i][j]),
					"button_press_event",
					G_CALLBACK(button_press),
					(gpointer)grids[i][j]);

			
			/*
			color.red = (int)(grids[i][j]->weights[0] * 65535);
			color.blue = (int)(grids[i][j]->weights[1] * 65535);
			color.green = (int)(grids[i][j]->weights[2] * 65535);
			*/
			color.red = (int)65535;
			color.blue = (int)65535;
			color.green = (int)65535;

			gtk_widget_modify_bg(drawingsele[i][j], GTK_STATE_NORMAL, &color);
			
			/* Set size of drawingarea */
			gtk_widget_set_size_request(GTK_WIDGET(drawingsele[i][j]), 10, 10);
			
			/* Insert drawing area into the upper left quadrant of the table */
			gtk_table_attach_defaults(GTK_TABLE(table), drawingsele[i][j], i, i+1, j, j+1);

			gtk_widget_show(drawingsele[i][j]);
		}
	}

	return table;
}

/*
 * Training UI 
 */
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







/*
 * Main 
 */

gint main(gint argc, gchar * argv[]) {
	GtkWidget *window;
	GtkWidget *tablebox;
	GtkWidget *table;
	GtkWidget *tableaccess;
	GtkWidget *tablecount;
	GtkWidget *separator;
	GtkWidget *mainbox;
	GtkWidget *tablesbox;
	GtkWidget *buttonbox;
	GtkWidget *button;
	GtkWidget *buttonrun;
	GtkWidget *buttonstop;
	GtkWidget *buttonreset;
	GtkWidget *buttontrainpercentual;
	GtkWidget *percentualbox;
	GtkWidget *conbox;
	GtkWidget *varbox;
	GtkWidget *taxbox;

	/*
	 * Label objects 
	 * */
	GtkWidget *ltablebox; 
	GtkWidget *labeltrained; 
	GtkWidget *labelfrequen; 
	GtkWidget *labelvisited; 
	GtkWidget *labelconlow; 
	GtkWidget *labelconmed; 
	GtkWidget *labelconhig; 
	GtkWidget *labelvarllow; 
	GtkWidget *labelvarlmed; 
	GtkWidget *labelvarlhig; 
	GtkWidget *labelvarplow; 
	GtkWidget *labelvarpmed; 
	GtkWidget *labelvarphig; 
	GtkWidget *labeltaxllow; 
	GtkWidget *labeltaxlmed; 
	GtkWidget *labeltaxlhig; 
	GtkWidget *labeltaxplow; 
	GtkWidget *labeltaxpmed; 
	GtkWidget *labeltaxphig; 


	stop=0; 
	int i=0; 
	
	/* Initiate grids */
	trained=read_trained_som("saved_som.txt", grids); 
	if (!trained) {
		init_grid(grids); 
	}


	/* Initialize the toolkit, remove gtk-related commandline stuff */
	gtk_init(&argc, &argv);
	
	/* Create toplevel window, set title and policies */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "SOM - Self Organizing Maps");
	gtk_container_set_border_width (GTK_CONTAINER (window), 4);

	/* Attach to the "delete" and "destroy" events so we can exit */
	/*g_signal_connect(GTK_OBJECT(window), "delete_event",
			 GTK_SIGNAL_FUNC(destroy_window), (gpointer)window);*/
  
  	/*Mainbox - vbox */
	mainbox = gtk_vbox_new(FALSE, 5);

	gtk_container_add(GTK_CONTAINER(window), mainbox);

	/*Horizontal - hbox ltablebox*/
	ltablebox = gtk_hbox_new(FALSE,5);
	gtk_box_pack_start(GTK_BOX(mainbox), ltablebox, TRUE, FALSE, 0);
	/*Defining labels*/
	labeltrained=gtk_label_new("SOM visualization"); 
	gtk_container_add(GTK_CONTAINER(ltablebox), labeltrained);
	labelfrequen=gtk_label_new("Frequence of visit"); 
	gtk_container_add(GTK_CONTAINER(ltablebox), labelfrequen);
	labelvisited=gtk_label_new("Visited area"); 
	gtk_container_add(GTK_CONTAINER(ltablebox), labelvisited);
	gtk_widget_show(labeltrained);
	gtk_widget_show(labelfrequen);
	gtk_widget_show(labelvisited);
	gtk_widget_show(ltablebox); 



  	/*Horizontal - hbox*/
	tablesbox = gtk_hbox_new(FALSE,5);

	gtk_box_pack_start(GTK_BOX(mainbox), tablesbox, TRUE, FALSE, 0);

	/*Tablebox - vbox*/
	tablebox = gtk_vbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(tablesbox), tablebox, TRUE, FALSE, 0);
	
	/* Create and initiate gtk table*/

	table = create_gtk_table(table, tablesbox);
	
	gtk_widget_show(table);

	tableaccess = create_gtk_freq_table(tableaccess, tablesbox);
	
	gtk_widget_show(tableaccess);

	tablecount = create_gtk_sele_table(tablecount, tablesbox);	

	gtk_widget_show(tablecount);
   
	gtk_widget_show(tablebox);

	gtk_widget_show(tablesbox);

	
	/*Statistic analyse*/

  	/*Horizontal - hbox*/
	percentualbox = gtk_hbox_new(FALSE,5);

	gtk_box_pack_start(GTK_BOX(mainbox), percentualbox, TRUE, FALSE, 0);

   
  	/*Conbox - vbox */
	conbox = gtk_vbox_new(FALSE, 5);

	gtk_container_add(GTK_CONTAINER(percentualbox), conbox);
	
	/*Consumo : Progress bar */
	for(i=0;i<3;i++){
		/*Defining labels*/
		if (i==0)
		{
			labelconlow=gtk_label_new("size of physical usage: low"); 
			gtk_container_add(GTK_CONTAINER(conbox), labelconlow);
			gtk_widget_show(labelconlow);
		}
		else
		{	
			if (i==1)
			{
			labelconmed=gtk_label_new("size of physical usage: media"); 
			gtk_container_add(GTK_CONTAINER(conbox), labelconmed);
			gtk_widget_show(labelconmed); 
			}
			else 
			{
			labelconhig=gtk_label_new("size of physical usage: high"); 
			gtk_container_add(GTK_CONTAINER(conbox), labelconhig);
			gtk_widget_show(labelconhig); 
			}
		}

		car_con[i]=0; 
		con[i] = gtk_progress_bar_new ();

		gtk_box_pack_start(GTK_BOX (conbox), con[i], TRUE, FALSE, 0);

		gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(con[i]),GTK_PROGRESS_LEFT_TO_RIGHT); 

		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(con[i]),0.0); 
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR(con[i]),"0.0 %"); 

		gtk_widget_show(con[i]);

	}
	

  	/*varbox - vbox */
	varbox = gtk_vbox_new(FALSE, 5);

	gtk_container_add(GTK_CONTAINER(percentualbox), varbox);

	/*Variacao : Progress bar */
	for(i=0;i<6;i++){
		/*Defining labels*/
		if (i==0)
		{
			labelvarllow=gtk_label_new("Memory usage variation: -low"); 
			gtk_container_add(GTK_CONTAINER(varbox), labelvarllow);
			gtk_widget_show(labelvarllow);
		}
		else
		{	
			if (i==1)
			{
				labelvarlmed=gtk_label_new("Memory usage variation: -media"); 
				gtk_container_add(GTK_CONTAINER(varbox), labelvarlmed);
				gtk_widget_show(labelvarlmed); 
			}
			else 
			{
				if (i==2)
				{
					labelvarlhig=gtk_label_new("Memory usage variation: -high"); 
					gtk_container_add(GTK_CONTAINER(varbox), labelvarlhig);
					gtk_widget_show(labelvarlhig); 

				}
				else 
				{
					if(i==3)
					{
						labelvarplow=gtk_label_new("Memory usage variation: +low"); 
						gtk_container_add(GTK_CONTAINER(varbox), labelvarplow);
						gtk_widget_show(labelvarplow); 
					}
					else
					{
						if(i==4)
						{
							labelvarpmed=gtk_label_new("Memory usage variation: +med"); 
							gtk_container_add(GTK_CONTAINER(varbox), labelvarpmed);
							gtk_widget_show(labelvarpmed); 
						}
						else 
						{
							labelvarphig=gtk_label_new("Memory usage variation: +high"); 
							gtk_container_add(GTK_CONTAINER(varbox), labelvarphig);
							gtk_widget_show(labelvarphig); 
						}
					}
				}
			}
		}

		car_var[i]=0; 
		var[i] = gtk_progress_bar_new ();

		gtk_box_pack_start(GTK_BOX (varbox), var[i], TRUE, FALSE, 0);

		gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(var[i]),GTK_PROGRESS_LEFT_TO_RIGHT); 

		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(var[i]),0.0); 
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR(var[i]),"0.0 %"); 

		gtk_widget_show(var[i]);

	}


  	/*Taxbox - vbox */
	taxbox = gtk_vbox_new(FALSE, 5);

	gtk_container_add(GTK_CONTAINER(percentualbox), taxbox);


	/*Taxa : Progress bar */
	for(i=0;i<6;i++){
		/*Defining labels*/
		if (i==0)
		{
			labeltaxllow=gtk_label_new("Rate of memory usage variation: -low"); 
			gtk_container_add(GTK_CONTAINER(taxbox), labeltaxllow);
			gtk_widget_show(labeltaxllow);
		}
		else
		{	
			if (i==1)
			{
				labeltaxlmed=gtk_label_new("Rate of memory usage variation: -media"); 
				gtk_container_add(GTK_CONTAINER(taxbox), labeltaxlmed);
				gtk_widget_show(labeltaxlmed); 
			}
			else 
			{
				if (i==2)
				{
					labeltaxlhig=gtk_label_new("Rate of memory usage variation: -high"); 
					gtk_container_add(GTK_CONTAINER(taxbox), labeltaxlhig);
					gtk_widget_show(labeltaxlhig); 

				}
				else 
				{
					if(i==3)
					{
						labeltaxplow=gtk_label_new("Rate of memory usage variation:  +low"); 
						gtk_container_add(GTK_CONTAINER(taxbox), labeltaxplow);
						gtk_widget_show(labeltaxplow); 
					}
					else
					{
						if(i==4)
						{
							labeltaxpmed=gtk_label_new("Rate of memory usage variation: +med"); 
							gtk_container_add(GTK_CONTAINER(taxbox), labeltaxpmed);
							gtk_widget_show(labeltaxpmed); 
						}
						else 
						{
							labeltaxphig=gtk_label_new("Rate of memory usage variation:  +high"); 
							gtk_container_add(GTK_CONTAINER(taxbox), labeltaxphig);
							gtk_widget_show(labeltaxphig); 
						}
					}
				}
			}
		}

		car_tax[i]=0; 
		
		tax[i] = gtk_progress_bar_new ();

		gtk_box_pack_start(GTK_BOX (taxbox), tax[i], TRUE, FALSE, 0);

		gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(tax[i]),GTK_PROGRESS_LEFT_TO_RIGHT); 

		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(tax[i]),0.0); 
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR(tax[i]),"0.0 %"); 

		gtk_widget_show(tax[i]);

	}



	gtk_widget_show(conbox); 
	gtk_widget_show(varbox); 
	gtk_widget_show(taxbox); 
	gtk_widget_show(percentualbox); 

  	/*Separator - hseparator*/
	separator = gtk_hseparator_new ();

	gtk_widget_set_size_request (separator, 400, 5);

	gtk_box_pack_start(GTK_BOX (mainbox), separator, TRUE, FALSE, 0);

	gtk_widget_show(separator);

	/*ButtonBox - Horizontal */

	buttonbox = gtk_hbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(mainbox), buttonbox, TRUE, FALSE, 0);

	/*Training ...*/
	button = gtk_button_new_with_label("Training");

	g_signal_connect(G_OBJECT(button), "clicked",
			 G_CALLBACK(repaint_gtk_table), NULL);

	gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, FALSE, 0);

	gtk_widget_show(button);

	
	/*Run...*/
	buttonrun = gtk_button_new_with_label("Run logfile");

	g_signal_connect(G_OBJECT(buttonrun), "clicked",
			 G_CALLBACK(select_log_file), NULL);

	gtk_box_pack_start(GTK_BOX(buttonbox), buttonrun, TRUE, FALSE, 0);

	gtk_widget_show(buttonrun);

	/*Stop...*/
	buttonstop = gtk_button_new_with_label("Stop logfile");

	g_signal_connect(G_OBJECT(buttonstop), "clicked",
			 G_CALLBACK(stop_logfile), NULL);

	gtk_box_pack_start(GTK_BOX(buttonbox), buttonstop, TRUE, FALSE, 0);

	gtk_widget_show(buttonstop);


	/*Reset...*/
	buttonreset = gtk_button_new_with_label("Reset");

	g_signal_connect(G_OBJECT(buttonreset), "clicked",
			 G_CALLBACK(reset_gtk_table), NULL);

	gtk_box_pack_start(GTK_BOX(buttonbox), buttonreset, TRUE, FALSE, 0);

	gtk_widget_show(buttonreset);

	/*Show percentual training...*/
	buttontrainpercentual = gtk_button_new_with_label("Trained %");

	g_signal_connect(G_OBJECT(buttontrainpercentual), "clicked",
			 G_CALLBACK(trained_percentual), NULL);

	gtk_box_pack_start(GTK_BOX(buttonbox), buttontrainpercentual, TRUE, FALSE, 0);

	gtk_widget_show(buttontrainpercentual);





	gtk_widget_show(buttonbox);

	/*Progress bar */
	pbar = gtk_progress_bar_new ();

	gtk_box_pack_start(GTK_BOX (mainbox), pbar, TRUE, FALSE, 0);
	
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),GTK_PROGRESS_LEFT_TO_RIGHT); 

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(pbar),0.0); 
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR(pbar),"0.0 %"); 
	
	gtk_widget_show(pbar);


	/*Status bar */
	status_bar = gtk_statusbar_new ();

	gtk_box_pack_start(GTK_BOX (mainbox), status_bar, TRUE, FALSE, 0);

	gtk_widget_show(status_bar);

	context_id = gtk_statusbar_get_context_id(
	                      GTK_STATUSBAR (status_bar), "Statusbar example");



	gtk_widget_show(mainbox);

	gtk_widget_show(window);

	



	/* Enter the gtk main loop (this never returns) */	
	gtk_main();
	
	return 0;
}
