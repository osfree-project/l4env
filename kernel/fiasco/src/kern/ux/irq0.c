
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

int main (void) {

  sigset_t blocked;
  struct itimerval t;
  int sig;

  signal (SIGINT, SIG_IGN);

  sigemptyset (&blocked);
  sigaddset   (&blocked, SIGALRM);
  sigprocmask (SIG_BLOCK, &blocked, NULL);

  t.it_interval.tv_sec  = t.it_value.tv_sec  = 0;
  t.it_interval.tv_usec = t.it_value.tv_usec = 10000;
  setitimer (ITIMER_REAL, &t, NULL);

  while (sigwait (&blocked, &sig) == 0)
    if (write (0, "T", 1) == -1)
      break;

  return 0;
}
