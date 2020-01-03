CC = gcc
CFLAGS = -Wall -ansi -pthread
EXEC = main

all: $(EXEC)

main: main.o
	$(CC) -o main main.o $(CFLAGS)

main.o: main.c
	$(CC) -o main.o -c main.c $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
