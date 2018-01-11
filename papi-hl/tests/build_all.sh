#!/bin/bash

CC=gcc
MPICC=mpicc
PAPI_FLAGS="-I${PAPI_DIR}/include -L${PAPI_DIR}/lib -lpapi"
OMP_FLAGS=-fopenmp

#serial
$CC serial.c $PAPI_FLAGS -o serial

#mpi_do_flops
$MPICC mpi_do_flops.c $PAPI_FLAGS -o mpi_do_flops

#omp
$CC omp.c $PAPI_FLAGS $OMP_FLAGS -o omp

#pthreads
$CC pthread.c $PAPI_FLAGS -lpthread -o pthread

#hybrid (mpi+openmp)
$MPICC mpi_omp.c $PAPI_FLAGS $OMP_FLAGS -o mpi_omp