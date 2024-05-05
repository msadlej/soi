CC = gcc
CFLAGS = -Wall -Wextra

all: main

main: main.o disc.o helpers.o
	$(CC) $(CFLAGS) -o main main.o disk.o helpers.o

main.o: main.c types.h
	$(CC) $(CFLAGS) -c main.c

disc.o: disk.c types.h
	$(CC) $(CFLAGS) -c disk.c

helpers.o: helpers.c types.h
	$(CC) $(CFLAGS) -c helpers.c

clean:
	rm -f main *.o
