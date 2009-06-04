#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>


GtkWidget *drawingarea[GRIDS_XSIZE][GRIDS_YSIZE];
GtkWidget *drawingfreq[GRIDS_XSIZE][GRIDS_YSIZE];
GtkWidget *drawingsele[GRIDS_XSIZE][GRIDS_YSIZE];
GdkColor color;

GtkWidget *status_bar; 
GtkWidget *pbar; 

gint context_id;  
gint trained; 
gchar *dirname;

int length;
gchar *filename; 
gint stop; 

GtkWidget *con[3]; 
GtkWidget *var[6]; 
GtkWidget *tax[6]; 

unsigned long car_con[3]; 
unsigned long car_var[6]; 
unsigned long car_tax[6]; 


