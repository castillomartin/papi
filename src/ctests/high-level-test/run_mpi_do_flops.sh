#!/bin/bash

OUTPUT_SCRIPT=$HOME/sources/papi/scripts/papi_output_writer.py
OUTPUT_DIRECTORY=/scratch/shared/fwinkler
TEST_PATH=`pwd`

generate_json_output()
{
   cd $PAPI_OUTPUT_DIRECTORY
   $OUTPUT_SCRIPT --source=papi
   cat papi.json
   echo ""
   cat papi_sum.json
   echo ""
}

cd $TEST_PATH
export PAPI_OUTPUT_DIRECTORY=$OUTPUT_DIRECTORY/mpi_do_flops_default
rm -r $PAPI_OUTPUT_DIRECTORY
mpirun -n 2 ./mpi_do_flops
generate_json_output

cd $TEST_PATH
export PAPI_EVENTS="PAPI_L1_DCM"
export PAPI_DEFAULT_NONE=true
export PAPI_OUTPUT_DIRECTORY=$OUTPUT_DIRECTORY/mpi_do_flops_user_nodefault
rm -r $PAPI_OUTPUT_DIRECTORY
mpirun -n 2 ./mpi_do_flops
generate_json_output

cd $TEST_PATH
export PAPI_EVENTS="PAPI_L1_DCM"
export PAPI_DEFAULT_NONE=false
export PAPI_OUTPUT_DIRECTORY=$OUTPUT_DIRECTORY/mpi_do_flops_user_default
rm -r $PAPI_OUTPUT_DIRECTORY
mpirun -n 2 ./mpi_do_flops
generate_json_output