CC=gcc
CFLAGS=-I. -Wall -Wextra -g

all: sequential_min_max parallel_min_max run_sequential

sequential_min_max : utils.o find_min_max.o utils.h find_min_max.h sequential_min_max.c
	$(CC) -o sequential_min_max find_min_max.o utils.o sequential_min_max.c $(CFLAGS)

parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h parallel_min_max.c
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

run_sequential : run_sequential.o
	$(CC) -o run_sequential run_sequential.o $(CFLAGS)

run_sequential.o : run_sequential.c
	$(CC) -o run_sequential.o -c run_sequential.c $(CFLAGS)

utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o : utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

clean :
	rm -f utils.o find_min_max.o run_sequential.o sequential_min_max parallel_min_max run_sequential