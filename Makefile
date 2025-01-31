CC = gcc
CFLAGS = -Wall

all: yash

yash: main.o parse.o actions.o signals.o
	$(CC) $(CFLAGS) -o yash main.o parse.o actions.o signals.o -lreadline

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

parse.o: parse.c
	$(CC) $(CFLAGS) -c parse.c

actions.o: actions.c
	$(CC) $(CFLAGS) -c actions.c

signals.o: signals.c
	$(CC) $(CFLAGS) -c signals.c

clean:
	rm -f yash main.o parse.o actions.o signals.o