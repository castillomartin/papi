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
export PAPI_OUTPUT_DIRECTORY=$OUTPUT_DIRECTORY/serial_default
rm -r $PAPI_OUTPUT_DIRECTORY
./serial
generate_json_output

cd $TEST_PATH
export PAPI_EVENTS="PAPI_L1_DCM"
export PAPI_DEFAULT_NONE=true
export PAPI_OUTPUT_DIRECTORY=$OUTPUT_DIRECTORY/serial_user_nodefault
rm -r $PAPI_OUTPUT_DIRECTORY
./serial
generate_json_output

cd $TEST_PATH
export PAPI_EVENTS="PAPI_L1_DCM"
export PAPI_DEFAULT_NONE=false
export PAPI_OUTPUT_DIRECTORY=$OUTPUT_DIRECTORY/serial_user_default
rm -r $PAPI_OUTPUT_DIRECTORY
./serial
generate_json_output