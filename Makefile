CC = gcc
CFLAGS = -Wall

all: yash

yash: main.o parse.o
	$(CC) $(CFLAGS) -o yash main.o parse.o -lreadline

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

parse.o: parse.c
	$(CC) $(CFLAGS) -c parse.c

clean:
	rm -f yash main.o parse.o