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

/* Defined in region_sample.c */
extern void hl_init_thread(int tid, void *data);
extern void hl_fini_thread(int tid, void *data);
extern void hl_init_process(int *argc, char **argv, void *data);
extern void hl_fini_process(int stat, void *data);

/* Local variables required for per-thread signal handling */
static volatile int global_callback_count = 0;
static volatile int global_thread_count = 0;
static void (*global_callback_fn)(void) = NULL;
static int global_callback_sig = 0;

static inline void thread_callback_handler(void)
{
  assert(global_callback_fn != NULL);
  fprintf(stderr,"MON T%d: THREAD CALLBACK HANDLER %p\n",monitor_get_thread_num(),global_callback_fn);
  (*global_callback_fn)();
  __sync_add_and_fetch(&global_callback_count, 1);
}

static int thread_callback_sighandler(int sig, siginfo_t *ptr, void *what)
{
  fprintf(stderr,"MON T%d: THREAD SIGNAL HANDLER %d\n",monitor_get_thread_num(),sig);
  thread_callback_handler();
  return 0;
}

void *monitor_init_process(int *argc, char **argv, void *data)
{
  int flags = 0;
  static struct sigaction act;
  
  /* install signal handler for all threads */
  global_callback_sig = SIGRTMIN+1;
  if (monitor_sigaction(global_callback_sig,&thread_callback_sighandler,flags,&act) < 0)
    error(1, errno, "sigaction");

  hl_init_process(argc,argv,data);
  monitor_init_thread(0,data);

  return(data);
}

void monitor_fini_process(int how, void *data)
{
  monitor_fini_thread(data);
  hl_fini_process(how,data);
}

void *monitor_init_thread(int tid, void *data)
{
  hl_init_thread(tid,data);
  __sync_add_and_fetch(&global_thread_count, 1);

  return(data);
}

void monitor_fini_thread(void *data)
{
  hl_fini_thread(monitor_get_thread_num(),data);
  __sync_sub_and_fetch(&global_thread_count, 1);
}

static inline void monitor_wait_broadcast_signal(volatile int *global_callback_count, int total)
{
  while (1) {
    int tmp = __sync_add_and_fetch(global_callback_count, 0);
    fprintf(stderr,"%d vs %d\n",tmp,total);
    if (tmp >= total)
      return;
    sched_yield();
  }
}

void monitor_broadcast_signal_sync(void (*fn)(void))
{
  while (__sync_bool_compare_and_swap(&global_callback_count,global_callback_count,0) == 0);
  while (__sync_bool_compare_and_swap(&global_callback_fn,global_callback_fn,fn) == 0);

  while (monitor_broadcast_signal(global_callback_sig) != 0) sched_yield();
  monitor_wait_broadcast_signal(&global_callback_count, global_thread_count-1);
  thread_callback_handler();

  while (__sync_bool_compare_and_swap(&global_callback_count,global_callback_count,0) == 0);
  while (__sync_bool_compare_and_swap(&global_callback_fn,global_callback_fn,NULL) == 0);
}

