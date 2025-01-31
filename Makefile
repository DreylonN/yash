CC = gcc
CFLAGS = -Wall

all: yash

yash: main.o parse.o actions.o
	$(CC) $(CFLAGS) -o yash main.o parse.o actions.o -lreadline

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

parse.o: parse.c
	$(CC) $(CFLAGS) -c parse.c

actions.o: actions.c
	$(CC) $(CFLAGS) -c actions.c

clean:
	rm -f yash main.o parse.o actions.o