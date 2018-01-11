#include <stdio.h>
#include "papi.h"
#include <omp.h>
#include <mpi.h>

void do_flops(int n)
{
   int i;
   double c = 0.11;
   double a = 0.5;
   double b = 6.2;

   for (i=0; i < n; i++) 
      c += a * b;

}

main(int argc, char **argv) 
{
   int i, world_size;
   int nthreads, tid;


   MPI_Init(NULL, NULL);

   MPI_Comm_size(MPI_COMM_WORLD, &world_size);
   int world_rank;
   MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
   char processor_name[MPI_MAX_PROCESSOR_NAME];
   int name_len;
   MPI_Get_processor_name(processor_name, &name_len);
   printf("Hello world from processor %s, rank %d"
           " out of %d processors\n",
           processor_name, world_rank, world_size);

PAPI_region_begin("do_flops_from_main");
   do_flops(100000000);
PAPI_region_end("do_flops_from_main");

#pragma omp parallel private(nthreads, tid)
   {
      /* Obtain thread number */
      tid = omp_get_thread_num();
      printf("Hello from Thread %d from rank %d\n", tid, world_rank);
PAPI_region_begin("do_flops_from_threads");
      do_flops(100000000);
PAPI_region_end("do_flops_from_threads");
   }
   MPI_Finalize();
}
