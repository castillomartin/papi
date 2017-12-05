#!/bin/bash

FC=gfortran
PAPI_FLAGS="-I${PAPI_DIR}/include -L${PAPI_DIR}/lib -lpapi"
OMP_FLAGS=-fopenmp

#serial
$FC $FFLAGS hello.F $PAPI_FLAGS -o hello