#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "papihl.h"

int matmult()
{
  int m, n, p, q, c, d, k, sum = 0, cksum = 0;
   int first[10][10], second[10][10], multiply[10][10];

   m = 10;
   n = 10;

   srand(time(NULL));

   for (c = 0; c < m; c++)
      for (d = 0; d < n; d++)
      first[c][d] = rand() % 20;

   p = 10;
   q = 10;

   if (n != p) {
     printf("Matrices with entered orders can't be multiplied with each other.\n"); return -1; }

      for (c = 0; c < p; c++)
      for (d = 0; d < q; d++)
         second[c][d] = rand() % 20;

      for (c = 0; c < m; c++) {
      for (d = 0; d < q; d++) {
         for (k = 0; k < p; k++) {
            sum = sum + first[c][k]*second[k][d];
         }

         multiply[c][d] = sum;
	 cksum += multiply[c][d];
	 sum = 0;
      }
      }
      return(cksum);
}


int main(int argc, char **argv) 
{
   PAPI_region_begin("calc_1");
   printf("Sum matmul round 1: 0x%x\n",matmult());
   PAPI_region_end("calc_1");

   PAPI_region_begin("calc_2");
   printf("Sum matmul round 2: 0x%x\n",matmult());
   PAPI_region_end("calc_2");
}
