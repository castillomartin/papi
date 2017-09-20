#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include "region.h"

int main()
{
#pragma omp parallel
  { 
    printf("OMP T%d: In parallel region\n",omp_get_thread_num());
    usleep(1);
  }
  region_start("parallel region 2");
#pragma omp parallel
  { 
    printf("OMP T%d: In parallel region 2\n",omp_get_thread_num());
    usleep(1);
  }
  region_stop("parallel region 2");

  exit(0);
}

