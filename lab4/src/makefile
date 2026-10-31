CC = gcc
CFLAGS = -I. -Wall -Wextra -g

all: parallel_sum

parallel_sum: parallel_sum.c libsum.a utils.o
	$(CC) -o parallel_sum parallel_sum.c utils.o -L. -lsum -lpthread $(CFLAGS)

libsum.a: sum.o
	ar rcs libsum.a sum.o

sum.o: sum.c sum.h
	$(CC) -o sum.o -c sum.c $(CFLAGS)

utils.o: utils.c utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

clean:
	rm -f *.o *.a parallel_sum