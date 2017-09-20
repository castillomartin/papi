/* Author: Philip Mucci, University of Tennessee */
/* Mods: */
#include <stdio.h>
void __attribute__ ((weak))
region_start(char *data)
{
  printf("(default start region %s)\n",data);
}

void __attribute__ ((weak))
region_stop(char *data)
{
  printf("(default stop region %s)\n",data);
}

