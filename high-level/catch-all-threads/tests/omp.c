#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include "region.h"

int main()
{
  region_start("parallel region");
#pragma omp parallel
  { 
    printf("OMP T%d: In parallel region\n",omp_get_thread_num());
    usleep(1);
  }
  region_stop("parallel region");

  exit(0);
}

