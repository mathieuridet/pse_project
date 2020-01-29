CC = gcc
CFLAGS = -Wall -pthread
EXEC = main

all: $(EXEC) clean

main: main.o h.o
	$(CC) -o main main.o $(CFLAGS)

main.o: main.c
	$(CC) -o main.o -c main.c $(CFLAGS)

h.o: h.c
	$(CC) -o main.o -c main.c $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
