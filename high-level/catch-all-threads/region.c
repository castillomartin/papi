/* Author: Philip Mucci, University of Tennessee */
/* Mods: */

#include <limits.h>
#include <stdio.h>
#include <signal.h>
#include <error.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <sched.h>
#include <err.h>
#include "monitor.h"

/* New function */
extern void monitor_broadcast_signal_sync(void (*fn)(void));

/* Mailbox for global region name */
static char *global_region_name = NULL;
/* Counts for process */
static unsigned long long global_start_cnt = 0;
static unsigned long long global_diff_cnt = 0;
/* Counts for thread */
static __thread unsigned long long thread_start_cnt = 0;
static __thread unsigned long long thread_diff_cnt = 0;
/* Counts for region */
static __thread unsigned long long region_start_cnt = 0;
static __thread unsigned long long region_diff_cnt = 0;
static __thread char *region_name = NULL;

/* Sample timer, could be PAPI! */

static inline long long read_real_clock(void)
{
  long long ret = 0;
#ifdef __x86_64__
  do {
    unsigned int a, d;
    asm volatile ( "rdtsc":"=a" ( a ), "=d"( d ) );
    ( ret ) = ( ( long long ) a ) | ( ( ( long long ) d ) << 32 );
  }
  while ( 0 );
#else
  __asm__ __volatile__( "rdtsc":"=A"( ret ): );
#endif
  return ret;
}

static void thread_region_start(void)
{
  unsigned long long tmp = read_real_clock();
  fprintf(stderr,"MON T%d: start region (%s), region_start_cnt %lld, will be %lld\n",monitor_get_thread_num(),global_region_name,region_start_cnt,tmp);
  if (global_region_name == NULL) errx(1, "thread_region_start: error global_region_name == NULL");
  if (region_start_cnt != 0) warnx("thread_region_start: region_start_cnt != 0");
  region_start_cnt = tmp;
  region_name = strdup(global_region_name);
}

static void thread_region_stop(void)
{
  unsigned long long tmp = read_real_clock();
  fprintf(stderr,"MON T%d: stop region (%s), region_start_cnt %lld, will be %lld\n",monitor_get_thread_num(),region_name,region_start_cnt,0ULL);
  if (region_name == NULL) errx(1, "thread_region_stop: error region_name == NULL");
  if (region_start_cnt == 0) errx(1, "thread_region_stop: error region_start_cnt == 0");
  region_diff_cnt += tmp - region_start_cnt;
  region_start_cnt = 0;
}

void region_start(char *region)
{
  fprintf(stderr,"REGION STARTING: (%s)\n",region);
  global_region_name = region;
  monitor_broadcast_signal_sync(thread_region_start);
  fprintf(stderr,"REGION STARTED: (%s)\n",region);
}

void region_stop(char *region)
{
  fprintf(stderr,"REGION STOPPING: (%s)\n",region);
  monitor_broadcast_signal_sync(thread_region_stop);
  global_region_name = NULL;
  fprintf(stderr,"REGION STOPPED: (%s)\n",region);
}

void hl_init_thread(int tid, void *data)
{
  thread_start_cnt = read_real_clock();
  fprintf(stderr,"MON T%d: THREAD INIT %llu\n",tid,thread_start_cnt);
  /* Handle the case where the region is active already but this
     thread is being created after region_start */
  if (global_region_name) {
    warnx("hl_init_thread: thread %d starting after region_start",tid);
    thread_region_start();
  }
}

void hl_fini_thread(int tid, void *data)
{
  /* Handle the case where the region is already active or in the process
     of stopping, but this thread is now being destroyed */
  if (global_region_name && (region_start_cnt != 0)) {
    warnx("hl_fini_thread: thread %d exiting before region_stop",tid);
    thread_region_stop();
  }

  /* read end, diff */
  unsigned long long cur = read_real_clock();
  thread_diff_cnt = cur - thread_start_cnt;
  assert(cur > thread_start_cnt);
  /* save to global structure for dumping at end of process */
  fprintf(stderr,"MON T%d: THREAD FINI region %s %llu (%.2f%%) of thread total %llu \n",tid, region_name, region_diff_cnt, 100.0*(float)region_diff_cnt/(float)thread_diff_cnt, thread_diff_cnt);
}

void hl_init_process(int *argc, char **argv, void *data)
{
  global_start_cnt = read_real_clock();
  fprintf(stderr,"MON: PROCESS INIT %llu\n",global_start_cnt);
}

void hl_fini_process(int stat, void *data)
{
  global_diff_cnt = read_real_clock() - global_start_cnt;
  fprintf(stderr,"MON: PROCESS FINI total %llu\n",global_diff_cnt);
  /* read end, diff, save */
  /* dump thread data */
  //  fprintf(stderr,"end main\n");
}

