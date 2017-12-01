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
export PAPI_EVENTS="PAPI_L8_DCM"
export PAPI_OUTPUT_DIRECTORY=$OUTPUT_DIRECTORY/serial_wrong_event
rm -r $PAPI_OUTPUT_DIRECTORY
./serial

cd $TEST_PATH
export PAPI_EVENTS="PAPI_L1_DCM,PAPI_L1_ICA,PAPI_L2_ICA,PAPI_L3_ICA,PAPI_L1_ICR,PAPI_L2_ICR,PAPI_L3_ICR"
export PAPI_OUTPUT_DIRECTORY=$OUTPUT_DIRECTORY/serial_too_many_events
rm -r $PAPI_OUTPUT_DIRECTORY
./serial

cd $TEST_PATH
export PAPI_EVENTS="PAPI_L1_DCM,PAPI_L1_ICA,PAPI_L2_ICA,PAPI_L3_ICA,PAPI_L1_ICR,PAPI_L2_ICR,PAPI_L3_ICR"
export PAPI_DEFAULT_NONE=true
export PAPI_OUTPUT_DIRECTORY=$OUTPUT_DIRECTORY/serial_user_default
rm -r $PAPI_OUTPUT_DIRECTORY
./serial
generate_json_output