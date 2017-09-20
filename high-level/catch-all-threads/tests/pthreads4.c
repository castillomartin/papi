#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <err.h>
#include <unistd.h>
#include "region.h"

void *
Thread( void *arg )
{
  printf( "Thread %#x started\n", ( int ) pthread_self(  ) );
  usleep(*(int *)arg);
  return NULL;
}

int
main( int argc, char **argv )
{
  pthread_t e_th;
  int rc;
  int usecs1;

  region_start("parallel region");

  usecs1 = 3000000;
  rc = pthread_create( &e_th, NULL, Thread, ( void * ) &usecs1 );
  if ( rc ) 
    err(rc, "pthread_create");

  pthread_join( e_th, NULL );

  exit(0);
}
