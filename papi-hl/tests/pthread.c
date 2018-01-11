#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <papi.h>

#define NUM_THREADS 4

void do_flops(int n)
{
   int i;
   double c = 0.11;
   double a = 0.5;
   double b = 6.2;

   for (i=0; i < n; i++) 
      c += a * b;
}

void *PrintHello(void *threadid)
{
   long tid;
   tid = (long)threadid;

PAPI_region_begin("do_flops");
   do_flops(100000000);
PAPI_region_end("do_flops");

   pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
   int nthreads, tid;

   pthread_t threads[NUM_THREADS];
   int rc;
   long t;

   //NOTE:
   //to initialize pthread support we need to set the first regions
   //in the main thread

PAPI_region_begin("main");

   for(t=0;t<NUM_THREADS;t++){
      printf("In main: creating thread %ld\n", t);
      rc = pthread_create(&threads[t], NULL, PrintHello, (void *)t);
      if (rc){
         printf("ERROR; return code from pthread_create() is %d\n", rc);
         exit(-1);
         }
      }
PAPI_region_end("main");
   /* Last thing that main() should do */
   pthread_exit(NULL);
}

