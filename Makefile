CFLAGS = -g -Wall -std=c99 -pedantic -o

all:		app

fork: 		fork.c main.h
		gcc $(CFLAGS) bin/run_fork fork.c

 
app:		main.c main.h
		gcc $(CFLAGS) bin/run main.c

clean:
		rm -f *~ run

