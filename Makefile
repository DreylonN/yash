CC = gcc
CFLAGS = -Wall

all: yash

yash: main.o parse.o actions.o signals.o jobs.o
	$(CC) $(CFLAGS) -o yash main.o parse.o actions.o signals.o jobs.o -lreadline

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

parse.o: parse.c
	$(CC) $(CFLAGS) -c parse.c

actions.o: actions.c
	$(CC) $(CFLAGS) -c actions.c

signals.o: signals.c
	$(CC) $(CFLAGS) -c signals.c

jobs.o: jobs.c jobs.h
	$(CC) $(CFLAGS) -c jobs.c

clean:
	rm -f yash main.o parse.o actions.o signals.o jobs.o