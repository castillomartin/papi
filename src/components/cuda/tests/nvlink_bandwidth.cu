/* 
 * Copyright 2015-2016 NVIDIA Corporation. All rights reserved.
 *
 * Sample to demonstrate use of NVlink CUPTI APIs
 * 
 * This version is significantly changed to use PAPI and the CUDA component to
 * handle access and reporting. As of 10/05/2018, I have deleted all CUPTI_ONLY
 * references, for clarity. The file nvlink_bandwidth_cupti_only.cu contains
 * the cupti-only code.  I also deleted the #if PAPI; there is no option
 * without PAPI.  Also, before my changes, the makefile did not even have a
 * build option that set CUPTI_ONLY for this file.
 *
 * -TonyC. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cuda.h>
#include <cupti.h>
#include "papi.h"

// THIS MACRO EXITS if the papi call does not return PAPI_OK. Do not use for routines that
// return anything else; e.g. PAPI_num_components, PAPI_get_component_info, PAPI_library_init.
#define CALL_PAPI_OK(papi_routine)                                                        \
    do {                                                                                  \
        int _papiret = papi_routine;                                                      \
        if (_papiret != PAPI_OK) {                                                        \
            fprintf(stderr, "%s:%d: PAPI Error: function %s failed with ret=%d [%s].\n"), \
                    __FILE__, __LINE__, #papi_routine, _papiret, PAPI_strerror(_papiret); \
            exit(-1);                                                                     \
        }                                                                                 \
    } while (0);


#define CUPTI_CALL(call)                                                \
    do {                                                                \
        CUptiResult _status = call;                                     \
        if (_status != CUPTI_SUCCESS) {                                 \
            const char *errstr;                                         \
            cuptiGetResultString(_status, &errstr);                     \
            fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n", \
                    __FILE__, __LINE__, #call, errstr);                 \
            exit(-1);                                                   \
        }                                                               \
    } while (0);  

#define DRIVER_API_CALL(apiFuncCall)                                    \
    do {                                                                \
        CUresult _status = apiFuncCall;                                 \
        if (_status != CUDA_SUCCESS) {                                  \
            const char *errName=NULL, *errStr=NULL;                     \
            CUresult _e1 = cuGetErrorName(_status, &errName);           \
            CUresult _e2 = cuGetErrorString(_status, &errStr);          \
            fprintf(stderr, "%s:%d: error: function %s failed with error %d [%s]='%s'.\n", \
                    __FILE__, __LINE__, #apiFuncCall, _status, errName, errStr);           \
            exit(-1);                                                   \
        }                                                               \
    } while (0);  

#define RUNTIME_API_CALL(apiFuncCall)                                   \
    do {                                                                \
        cudaError_t _status = apiFuncCall;                              \
        if (_status != cudaSuccess) {                                   \
            fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n", \
                    __FILE__, __LINE__, #apiFuncCall, cudaGetErrorString(_status)); \
            exit(-1);                                                   \
        }                                                               \
    } while (0);  

#define MEMORY_ALLOCATION_CALL(var)                                     \
    do {                                                                \
        if (var == NULL) {                                              \
            fprintf(stderr, "%s:%d: Error: Memory Allocation Failed \n", \
                    __FILE__, __LINE__);                                \
            exit(-1);                                                   \
        }                                                               \
    } while (0);  


#define MAX_DEVICES    (32)
#define BLOCK_SIZE     (1024)
#define GRID_SIZE      (512)
#define BUF_SIZE       (32 * 1024)
#define ALIGN_SIZE     (8)
#define SUCCESS        (0)
#define NUM_METRIC     (4)
#define NUM_EVENTS     (2)
#define MAX_SIZE       (64*1024*1024)   // 64 MB
#define NUM_STREAMS    (6)      // gp100 has 6 physical copy engines

int cpuToGpu = 0;
int gpuToGpu = 0;


//-----------------------------------------------------------------------------
// This is the GPU routine to move a block from dst (on one GPU) to src (on
// another GPU. 
//-----------------------------------------------------------------------------
extern "C" __global__ void test_nvlink_bandwidth(float *src, float *dst)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    dst[idx] = src[idx] * 2.0f;
} // end routine

#define DIM(x) (sizeof(x)/sizeof(*(x)))


//-----------------------------------------------------------------------------
// Return a text version with B, KB, MB, GB or TB. 
//-----------------------------------------------------------------------------
void calculateSize(char *result, uint64_t size)
{
    int i;

    const char *sizes[] = { "TB", "GB", "MB", "KB", "B" };
    uint64_t exbibytes = 1024ULL * 1024ULL * 1024ULL * 1024ULL;

    uint64_t multiplier = exbibytes;

    for(i = 0; (unsigned) i < DIM(sizes); i++, multiplier /= (uint64_t) 1024) {
        if(size < multiplier)
            continue;
        sprintf(result, "%.1f %s", (float) size / multiplier, sizes[i]);
        return;
    }
    strcpy(result, "0");
    return;
} // end routine


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void testCpuToGpu(CUpti_EventGroup * eventGroup, 
      CUdeviceptr * pDevBuffer, float **pHostBuffer, size_t bufferSize, 
      cudaStream_t * cudaStreams, uint64_t * timeDuration, 
      int numEventGroup)
{
    int i;
    fprintf(stderr, "NUM_STREAMS = %d.\n", NUM_STREAMS); 
    // Unidirectional copy H2D (Host to Device).
    for(i = 0; i < NUM_STREAMS; i++) {
        RUNTIME_API_CALL(cudaMemcpyAsync((void *) pDevBuffer[i], pHostBuffer[i], bufferSize, cudaMemcpyHostToDevice, cudaStreams[i]));
    }
    RUNTIME_API_CALL(cudaDeviceSynchronize());

    // Unidirectional copy D2H (Device to Host).
    for(i = 0; i < NUM_STREAMS; i++) {
        RUNTIME_API_CALL(cudaMemcpyAsync(pHostBuffer[i], (void *) pDevBuffer[i], bufferSize, cudaMemcpyDeviceToHost, cudaStreams[i]));
    }
    RUNTIME_API_CALL(cudaDeviceSynchronize());

    // Bidirectional copy
    for(i = 0; i < NUM_STREAMS; i += 2) {
        RUNTIME_API_CALL(cudaMemcpyAsync((void *) pDevBuffer[i], pHostBuffer[i], bufferSize, cudaMemcpyHostToDevice, cudaStreams[i]));
        RUNTIME_API_CALL(cudaMemcpyAsync(pHostBuffer[i + 1], (void *) pDevBuffer[i + 1], bufferSize, cudaMemcpyDeviceToHost, cudaStreams[i + 1]));
    }
    RUNTIME_API_CALL(cudaDeviceSynchronize());
} // end routine.


//-----------------------------------------------------------------------------
// Copy buffers from the host to each device, in preparation for a transfer
// between devices.
//-----------------------------------------------------------------------------
void testGpuToGpu_part1(CUpti_EventGroup * eventGroup, 
      CUdeviceptr * pDevBuffer0, CUdeviceptr * pDevBuffer1, 
      float **pHostBuffer, size_t bufferSize, 
      cudaStream_t * cudaStreams, uint64_t * timeDuration, 
      int numEventGroup)
{
    int i;

    RUNTIME_API_CALL(cudaSetDevice(0));
    RUNTIME_API_CALL(cudaDeviceEnablePeerAccess(1, 0));
    RUNTIME_API_CALL(cudaSetDevice(1));
    RUNTIME_API_CALL(cudaDeviceEnablePeerAccess(0, 0));

    // Unidirectional copy H2D
    for(i = 0; i < NUM_STREAMS; i++) {
        RUNTIME_API_CALL(cudaMemcpyAsync((void *) pDevBuffer0[i], pHostBuffer[i], bufferSize, cudaMemcpyHostToDevice, cudaStreams[i]));
    }

    RUNTIME_API_CALL(cudaDeviceSynchronize());

    for(i = 0; i < NUM_STREAMS; i++) {
        RUNTIME_API_CALL(cudaMemcpyAsync((void *) pDevBuffer1[i], pHostBuffer[i], bufferSize, cudaMemcpyHostToDevice, cudaStreams[i]));
    }

    RUNTIME_API_CALL(cudaDeviceSynchronize());
} // end routine.


//-----------------------------------------------------------------------------
// Copy from device zero to device 1, then from device 1 to device 0.
//-----------------------------------------------------------------------------
void testGpuToGpu_part2(CUpti_EventGroup * eventGroup, 
      CUdeviceptr * pDevBuffer0, CUdeviceptr * pDevBuffer1, 
      float **pHostBuffer, size_t bufferSize, 
      cudaStream_t * cudaStreams, uint64_t * timeDuration, 
      int numEventGroup)
{
    int i;

    for(i = 0; i < NUM_STREAMS; i++) {
        RUNTIME_API_CALL(cudaMemcpyAsync((void *) pDevBuffer0[i], (void *) pDevBuffer1[i], bufferSize, cudaMemcpyDeviceToDevice, cudaStreams[i]));
        //printf("Copy %zu stream %d to devBuffer0 from devBuffer1 \n", bufferSize, i);
    }
    RUNTIME_API_CALL(cudaDeviceSynchronize());

    for(i = 0; i < NUM_STREAMS; i++) {
        RUNTIME_API_CALL(cudaMemcpyAsync((void *) pDevBuffer1[i], (void *) pDevBuffer0[i], bufferSize, cudaMemcpyDeviceToDevice, cudaStreams[i]));
        // printf("Copy %zu stream %d to devBuffer0 from devBuffer1 \n", bufferSize, i);
    }
    RUNTIME_API_CALL(cudaDeviceSynchronize());

    for(i = 0; i < NUM_STREAMS; i++) {
        test_nvlink_bandwidth <<< GRID_SIZE, BLOCK_SIZE >>> ((float *) pDevBuffer1[i], (float *) pDevBuffer0[i]);
        // printf("test_nvlink_bandwidth stream %d \n", i);
    }
} // end routine.


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void printUsage()
{
    printf("usage: Demonstrate use of NVlink CUPTI APIs\n");
    printf("       -help           : display help message\n");
    printf("       --cpu-to-gpu    : Show results for data transfer between CPU and GPU \n");
    printf("       --gpu-to-gpu    : Show results for data transfer between two GPUs \n");
} // end routine.


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void parseCommandLineArgs(int argc, char *argv[])
{
    if(argc != 2) {
        printf("Invalid number of options\n");
        exit(0);
    }

    if(strcmp(argv[1], "--cpu-to-gpu") == 0) {
        cpuToGpu = 1;
    } else if(strcmp(argv[1], "--gpu-to-gpu") == 0) {
        gpuToGpu = 1;
    } else if((strcmp(argv[1], "--help") == 0) || 
              (strcmp(argv[1], "-help") == 0)  || 
              (strcmp(argv[1], "-h") == 0)) {
        printUsage();
        exit(0);
    } else {
        cpuToGpu = 1;
    }
} // end routine.


//-----------------------------------------------------------------------------
// Main program.
//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    int deviceCount = 0, i = 0, numEventGroup = 0;
    size_t bufferSize = 0, freeMemory = 0, totalMemory = 0;
    char str[64];

    CUdeviceptr pDevBuffer0[NUM_STREAMS];
    CUdeviceptr pDevBuffer1[NUM_STREAMS];
    float *pHostBuffer[NUM_STREAMS];

    cudaStream_t cudaStreams[NUM_STREAMS] = { 0 };
    cudaDeviceProp prop[MAX_DEVICES];
    uint64_t timeDuration;
    CUpti_EventGroup eventGroup[32];

    const char *EventBase[NUM_METRIC] = {
         "cuda:::metric:nvlink_total_data_transmitted"
        ,"cuda:::metric:nvlink_total_data_received"
        ,"cuda:::metric:nvlink_transmit_throughput"
        ,"cuda:::metric:nvlink_receive_throughput"     // We can read these four together, no more.
//      ,"cuda:::metric:nvlink_total_response_data_received"
//      ,"cuda:::metric:nvlink_user_response_data_received"
//      ,"cuda:::metric:nvlink_user_ratom_data_transmitted"
//      ,"cuda:::metric:nvlink_user_nratom_data_transmitted"
//      ,"cuda:::metric:nvlink_overhead_data_transmitted"
//      ,"cuda:::metric:nvlink_data_transmission_efficiency"
//      ,"cuda:::metric:nvlink_user_data_transmitted"
//      ,"cuda:::metric:nvlink_user_data_received"
//      ,"cuda:::metric:nvlink_overhead_data_received"
//      ,"cuda:::metric:nvlink_total_nratom_data_transmitted"
//      ,"cuda:::metric:nvlink_total_ratom_data_transmitted"
//      ,"cuda:::metric:nvlink_total_write_data_transmitted"
//      ,"cuda:::metric:nvlink_user_write_data_transmitted"
//      ,"cuda:::metric:nvlink_data_receive_efficiency"
    };
    
    // Parse command line arguments
    parseCommandLineArgs(argc, argv);

//  CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_NVLINK));
//  CUPTI_CALL(cuptiActivityRegisterCallbacks(bufferRequested, bufferCompleted));


    DRIVER_API_CALL(cuInit(0));
    RUNTIME_API_CALL(cudaGetDeviceCount(&deviceCount));
    printf("There are %d devices.\n", deviceCount);

    if(deviceCount == 0) {
        printf("There is no device supporting CUDA.\n");
        exit(-1);
    }

    for(i = 0; i < deviceCount; i++) {
        RUNTIME_API_CALL(cudaGetDeviceProperties(&prop[i], i));
        printf("CUDA Device %d Name: %s", i, prop[i].name);
        printf(", MultiProcessors=%i", prop[i].multiProcessorCount);
        printf(", MaxThreadsPerMP=%i", prop[i].maxThreadsPerMultiProcessor);
        printf("\n");
    }

    // Set memcpy size based on available device memory
    RUNTIME_API_CALL(cudaMemGetInfo(&freeMemory, &totalMemory));
    printf("Total Device Memory available : ");
    calculateSize(str, (uint64_t) totalMemory);
    printf("%s\n", str);

    bufferSize = MAX_SIZE < (freeMemory / 4) ? MAX_SIZE : (freeMemory / 4);
    bufferSize = bufferSize/2;
    printf("Memcpy size is set to %llu B (%llu MB)\n", (unsigned long long) bufferSize, (unsigned long long) bufferSize / (1024 * 1024));

    for(i = 0; i < NUM_STREAMS; i++) {
        RUNTIME_API_CALL(cudaStreamCreate(&cudaStreams[i]));
    }
    RUNTIME_API_CALL(cudaDeviceSynchronize());

    // Nvlink-topology Records are generated even before cudaMemcpy API is called.
    CUPTI_CALL(cuptiActivityFlushAll(0));

    fprintf(stderr, "Setup PAPI counters internally (PAPI)\n");
    int EventSet = PAPI_NULL;
    long long values[MAX_DEVICES * NUM_METRIC];
    char *EventName[MAX_DEVICES * NUM_METRIC];
    int eventCount;
    int retval, ee;
    int k, cid=-1;

    /* PAPI Initialization */
    retval = PAPI_library_init(PAPI_VER_CURRENT);
    if(retval != PAPI_VER_CURRENT) {
        fprintf(stderr, "PAPI_library_init failed, ret=%i [%s]\n", 
            retval, PAPI_strerror(retval));
        exit(-1);
    }

    fprintf(stderr, "PAPI version: %d.%d.%d\n", 
        PAPI_VERSION_MAJOR(PAPI_VERSION), 
        PAPI_VERSION_MINOR(PAPI_VERSION), 
        PAPI_VERSION_REVISION(PAPI_VERSION));

    // Find cuda component index.
    k = PAPI_num_components();                                          // get number of components.
    for (i=0; i<k && cid<0; i++) {                                      // while not found,
        PAPI_component_info_t *aComponent = 
            (PAPI_component_info_t*) PAPI_get_component_info(i);        // get the component info.     
        if (aComponent == NULL) {                                       // if we failed,
            fprintf(stderr,  "PAPI_get_component_info(%i) failed, "
                "returned NULL. %i components reported.\n", i,k);
            exit(-1);    
        }

       if (strcmp("cuda", aComponent->name) == 0) cid=i;                // If we found our match, record it.
    } // end search components.

    if (cid < 0) {                                                      // if no PCP component found,
        fprintf(stderr, "Failed to find pcp component among %i "
            "reported components.\n", k);
        exit(-1); 
    }

    fprintf(stderr, "Found CUDA Component at id %d\n",cid);

    CALL_PAPI_OK(PAPI_create_eventset(&EventSet)); 
    CALL_PAPI_OK(PAPI_assign_eventset_component(EventSet, cid)); 

    // Add events at a GPU specific level ... eg cuda:::metric:nvlink_total_data_transmitted:device=0
    // Just profile devices to match the CUPTI example
    char tmpEventName[1024];
    eventCount = 0;
    for(i = 0; i < deviceCount; i++) {                                  // Profile all devices.
        fprintf(stderr, "Set device to %d\n", i);
        for(ee = 0; ee < NUM_METRIC; ee++) {
            snprintf(tmpEventName, 1024, "%s:device=%d\0", EventBase[ee], i);
            retval = PAPI_add_named_event(EventSet, tmpEventName);      // Don't want to fail program if name not found...
            if(retval == PAPI_OK) {
                fprintf(stderr, "Added event %s to GPU %d.\n", tmpEventName, i);
                EventName[eventCount] = strdup(tmpEventName);
                eventCount++;
            } else {
                fprintf(stderr, "Failed to add event %s to GPU %i; ret=%d [%s].\n", tmpEventName, i, retval, PAPI_strerror(retval));
            }
        }
    }

    if (eventCount < 1) {                                              // If we failed on all of them,
        fprintf(stderr, "Unable to add any NVLINK events; they are not present in the component.\n");
        fprintf(stderr, "Unable to proceed with this test.\n");
        CALL_PAPI_OK(PAPI_destroy_eventset(&EventSet));                // destroy the event set.
        exit(-1);                                                      // exit no matter what.
    }

    // If we have events, init the values to -1, to see if we read them. 
    for(i = 0; i < eventCount; i++)
        values[i] = -1;

    // ===== Allocate Memory =====================================

    for(i = 0; i < NUM_STREAMS; i++) {
        RUNTIME_API_CALL(cudaMalloc((void **) &pDevBuffer0[i], bufferSize));

        pHostBuffer[i] = (float *) malloc(bufferSize);
        MEMORY_ALLOCATION_CALL(pHostBuffer[i]);
    }
    
    if(cpuToGpu) {
        CALL_PAPI_OK(PAPI_start(EventSet));                             // Start event counters.
        testCpuToGpu(eventGroup, pDevBuffer0, pHostBuffer, bufferSize, cudaStreams, &timeDuration, numEventGroup);
        CALL_PAPI_OK(PAPI_stop(EventSet, values));                      // Stop and read values.
    } else if(gpuToGpu) {
        RUNTIME_API_CALL(cudaSetDevice(1));
        for(i = 0; i < NUM_STREAMS; i++) 
            RUNTIME_API_CALL(cudaMalloc((void **) &pDevBuffer1[i], bufferSize));

        //  Prepare the copy, load up buffers on each device from the host.
        testGpuToGpu_part1(eventGroup, pDevBuffer0, pDevBuffer1, pHostBuffer, bufferSize, cudaStreams, &timeDuration, numEventGroup);
        CALL_PAPI_OK(PAPI_start(EventSet));                             // Start event counters.
        // Copy from device 0->1, then device 1->0.
        testGpuToGpu_part2(eventGroup, pDevBuffer0, pDevBuffer1, pHostBuffer, bufferSize, cudaStreams, &timeDuration, numEventGroup);
        CALL_PAPI_OK(PAPI_stop(EventSet, values));                      // Stop and read values.
    }

    // report each event counted.
    int eventsRead=0;
    for(i = 0; i < eventCount; i++) {
        char str[64];
        if (values[i] >= 0) {                                           // If not still -1,
            eventsRead++;                                               // .. count and report.
            calculateSize(str, (uint64_t) values[i] );
            printf("PAPI %s: %s \n", EventName[i], str);
        }
    }


    CALL_PAPI_OK(PAPI_cleanup_eventset(EventSet));                      // Delete all events in set.
    CALL_PAPI_OK(PAPI_destroy_eventset(&EventSet));                     // Release PAPI memory.
    PAPI_shutdown();                                                    // Has no return.
        
    if (eventsRead > 0) {                                               // If we succeeded with any, report. 
        printf("%i bandwidth events successfully reported.\n", eventsRead);
        return(0);                                                      // exit OK.
    }

    printf("Failed to read any bandwidth events.\n");                   // report a failure.
        
    return (-1);                                                        // Exit with error.
} // end MAIN.
