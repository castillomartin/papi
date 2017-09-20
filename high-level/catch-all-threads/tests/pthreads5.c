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
  pthread_t e_th, f_th;
  int rc;
  int usecs1, usecs2, usecs3;

  usecs1 = 3000000;
  rc = pthread_create( &e_th, NULL, Thread, ( void * ) &usecs1 );
  if ( rc ) 
    err(rc, "pthread_create");

  usecs2 = 2000000;
  rc = pthread_create( &f_th, NULL, Thread, ( void * ) &usecs2 );
  if ( rc ) 
    err(rc, "pthread_create");

  region_start("parallel region 2");

  usecs3 = 1000000;
  Thread( &usecs3 );

  region_stop("parallel region 2");

  pthread_join( e_th, NULL );
  pthread_join( f_th, NULL );

  exit(0);
}
