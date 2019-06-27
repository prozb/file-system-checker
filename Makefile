CFLAGS = -g -Wall -std=c99 -pedantic -o

all:		app
 
app:		main.c main.h
		gcc $(CFLAGS) bin/run main.c

clean:
		rm -rf bin/*

