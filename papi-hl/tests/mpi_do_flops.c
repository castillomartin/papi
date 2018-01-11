#include <mpi.h>
#include <stdio.h>
#include <papi.h>

void do_flops(int n)
{
    int i;
    double c = 0.11;
    double a = 0.5;
    double b = 6.2;

PAPI_region_begin("calc");
    for (i=0; i < n; i++) 
        c += a * b;
PAPI_region_end("calc");
}


int main(int argc, char** argv) {
    MPI_Init(NULL, NULL);
    int i, world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    printf("Hello world from processor %s, rank %d"
           " out of %d processors\n",
           processor_name, world_rank, world_size);

PAPI_region_begin("main");

   for (i=0;i<10;i++)
      do_flops(100000000);

PAPI_region_end("main");

   MPI_Finalize();
}
