#!/bin/bash

CC=mpicc
CFLAGS=-O3 -Wall -std=c99 -lm

OBJ=midpointrule_mpi_error_criterion.o

midpointrule_mpi_error_criterion: $(OBJ)
	${CC} ${CFLAGS} -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o midpointrule_mpi_error_criterion *~ 
