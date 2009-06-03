CC=gcc

PROG=main
SOM=som
UTIL=util
GTK_TRAINING=app_training
GTK_TRAINED=app_trained

CFLAGS=-Wall \
	-DG_DISABLE_DEPRECATED 	\
	-DGDK_DISABLE_DEPRECATED 	\
	-DGDK_PIXBUF_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED

all: $(GTK_TRAINING) $(GTK_TRAINED)

app_training: $(GTK_TRAINING).c 
	$(CC) $(CFLAGS) $(GTK_TRAINING).c $(PROG).c $(SOM).c $(UTIL).c -o \
	$(GTK_TRAINING) -lm \
	`pkg-config gtk+-2.0 --cflags --libs`

app_trained: $(GTK_TRAINED).c 
	$(CC) $(CFLAGS) $(GTK_TRAINED).c $(PROG).c $(SOM).c $(UTIL).c -o \
	$(GTK_TRAINED) -lm \
	`pkg-config gtk+-2.0 --cflags --libs`

clean:
	rm -f *.o
	rm -f *.so
	rm -f *.a
	rm -f $(GTK_TRAINING)
	rm -f $(GTK_TRAINED)