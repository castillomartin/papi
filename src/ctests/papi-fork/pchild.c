/*
 *  Test PAPI with fork() and exec().
 */

#include <sys/time.h>
#include <sys/types.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <papi.h>

#define MAX_EVENTS  3

int Event[MAX_EVENTS] = {
    PAPI_TOT_CYC,
    PAPI_FP_INS,
    PAPI_FAD_INS,
};

int Threshold[MAX_EVENTS] = {
    8000000,
    4000000,
    4000000,
};

int num_events = 1;
int EventSet = PAPI_NULL;

struct timeval start, last;
long count, total;

void
my_handler(int EventSet, void *pc, long long ovec, void *context)
{
    count++;
    total++;
}

void
zero_count(void)
{
    gettimeofday(&start, NULL);
    last = start;
    count = 0;
    total = 0;
}

#define HERE(str)  printf("[%d] %s\n", getpid(), str);

void
print_rate(char *str)
{
    struct timeval now;
    double st_secs, last_secs;

    gettimeofday(&now, NULL);
    st_secs = (double)(now.tv_sec - start.tv_sec)
	+ ((double)(now.tv_usec - start.tv_usec))/1000000.0;
    last_secs = (double)(now.tv_sec - last.tv_sec)
	+ ((double)(now.tv_usec - last.tv_usec))/1000000.0;
    if (last_secs <= 0.001)
	last_secs = 0.001;

    printf("[%d] %s, time = %.3f, total = %ld, last = %ld, rate = %.1f/sec\n",
	   getpid(), str, st_secs, total, count, ((double)count)/last_secs);

    count = 0;
    last = now;
}

void
do_cycles(int program_time)
{
    struct timeval start, now;
    double x, sum;

    gettimeofday(&start, NULL);

    for (;;) {
        sum = 1.0;
        for (x = 1.0; x < 250000.0; x += 1.0)
            sum += x;
        if (sum < 0.0)
            printf("==>>  SUM IS NEGATIVE !!  <<==\n");

        gettimeofday(&now, NULL);
        if (now.tv_sec >= start.tv_sec + program_time)
            break;
    }
}

void
my_papi_init(void)
{
    if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
        errx(1, "PAPI_library_init failed");
}

void
my_papi_start(void)
{
    int ev;

    EventSet = PAPI_NULL;

    if (PAPI_create_eventset(&EventSet) != PAPI_OK)
        errx(1, "PAPI_create_eventset failed");

    for (ev = 0; ev < num_events; ev++) {
        if (PAPI_add_event(EventSet, Event[ev]) != PAPI_OK)
            errx(1, "PAPI_add_event failed");
    }

    for (ev = 0; ev < num_events; ev++) {
        if (PAPI_overflow(EventSet, Event[ev], Threshold[ev], 0, my_handler)
            != PAPI_OK) {
            errx(1, "PAPI_overflow failed");
        }
    }

    if (PAPI_start(EventSet) != PAPI_OK)
        errx(1, "parent PAPI_start failed");
}

void
my_papi_stop(void)
{
    if (PAPI_stop(EventSet, NULL) != PAPI_OK)
        errx(1, "PAPI_stop failed, pid = %d", getpid());
}

void
run(char *str, int len)
{
    int n;

    print_rate(str);
    for (n = 1; n <= len; n++) {
	do_cycles(1);
	print_rate(str);
    }
}

int
main(int argc, char **argv)
{
    char buf[100];

    if (argc < 2 || sscanf(argv[1], "%d", &num_events) < 1)
	num_events = MAX_EVENTS;
    if (num_events < 0 || num_events > MAX_EVENTS)
	num_events = 1;

    printf("[%d] pchild: num_events = %d\n\n", getpid(), num_events);
    sprintf(buf, "%d", num_events);

    do_cycles(1);

    zero_count();
    my_papi_init();
    my_papi_start();

    run("pchild", 4);

    my_papi_stop();

    HERE("end pchild");

    return (0);
}