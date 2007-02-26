/* $Id$ */
/*****************************************************************************/
/**
 * \file
 * \brief  Idle cycle counter
 *
 * \date   03/18=2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* standard includes */
#include <stdio.h>
#include <string.h>

/* L4 includes */
#include <l4/sys/syscalls.h>
#include <l4/rmgr/librmgr.h>
#include <l4/util/rdtsc.h>

#define PRIO               2

#define ROUNDS             1000000000ULL
#define CYCLES_PER_ROUND   2     /* we assume that each round takes x cycles,
                                  * this seems to be true for AMD and up to 
                                  * Intel Pentium III, Pentium 4 seems to 
                                  * need about 1.2 (???) cycles per round,
                                  * set DO_CALIBRATE to 1 to run a 
                                  * calibration loop instead of using a fixed 
                                  * value */

#define HISTORY_SIZE       50

#define DO_CALIBRATE       1
#define CALIBRATE_ROUNDS   10


int
main(void)
{
  int hist,i;
  l4_cpu_time_t start, stop, diff,diff_orig,diff10,diff25,diff50;
  l4_cpu_time_t history[HISTORY_SIZE];
  double load,load10,load25,load50;
  double cycles_per_round;
  l4_cpu_time_t cycles_per_run;

  //rmgr_set_prio(l4_myself(),PRIO);

#if DO_CALIBRATE
  printf("l4lload: calibrating....\n");

  diff = 0;
  for (i = 0; i < CALIBRATE_ROUNDS; i++)
    {
      register int n asm ("edi");
      n = ROUNDS;

      start = l4_rdtsc();
      while(n--);
      stop = l4_rdtsc();

      diff += (stop - start);
    }
  cycles_per_round = 
    (double)diff / (double)(ROUNDS * CALIBRATE_ROUNDS);

  printf("l4lload: %8.6f cycles per round\n",cycles_per_round);
#else
  cycles_per_round = (double)CYCLES_PER_ROUND;
#endif
  cycles_per_run = (l4_cpu_time_t)(cycles_per_round * (double)ROUNDS);

  memset(history,0,HISTORY_SIZE * sizeof(l4_cpu_time_t));
  hist = 0;
  diff10 = diff25 = diff50 = 0;
  while (1)
    {
      register int n asm ("edi");
      n = ROUNDS;

      start = l4_rdtsc();
      while(n--);
      stop = l4_rdtsc();

      diff = stop - start;
      diff_orig = diff;
      if (diff < cycles_per_run)
        diff = cycles_per_run;

      diff10 = diff10 + diff - history[9];
      diff25 = diff25 + diff - history[24];
      diff50 = diff50 + diff - history[49];
      memmove(&history[1],&history[0],
              (HISTORY_SIZE - 1) * sizeof(l4_cpu_time_t));
      history[0] = diff;
      hist++;

      load = ((double)(diff - cycles_per_run)) / (double)diff;
      i = (hist > 10) ? 10 : hist;
      load10 = ((double)(diff10 - (i * cycles_per_run))) / (double)diff10;
      i = (hist > 25) ? 25 : hist;
      load25 = ((double)(diff25 - (i * cycles_per_run))) / (double)diff25;
      i = (hist > 50) ? 50 : hist;
      load50 = ((double)(diff50 - (i * cycles_per_run))) / (double)diff50;

      printf("%10lu (%10lu) cycles, load %6.4f/%6.4f/%6.4f/%6.4f\n",
             (unsigned long)diff,(unsigned long)diff_orig,
             load,load10,load25,load50);

    }

  /* done */
  return 0;
}
