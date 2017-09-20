#include <limits.h>
#include <stdio.h>
extern int monitor_real_system(char *);
extern void monitor_real_exit(int);

void monitor_fini_process(int how, void* data)
{
  char shell_cmd[PATH_MAX] = "echo > /dev/null";
  if (monitor_real_system(shell_cmd) != 0) {
    fprintf(stderr, "Error running %s\n", shell_cmd);
    monitor_real_exit (1);
  }
}
