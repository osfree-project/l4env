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
#include <stdlib.h>

/* L4 includes */
#include <l4/sys/syscalls.h>
#include <l4/rmgr/librmgr.h>
#include <l4/util/rdtsc.h>
#include <l4/util/parse_cmd.h>
#include <l4/log/l4log.h>

char LOG_tag[9]="l4load";

static int prio=2;
static unsigned long rounds;
static double cycles_per_round;	/* there seem to be processors where this
				 * is not an integer number */
static int cycles_per_round_int;/* cycles_per_round, given as an int as
				 * cmdline-arg */
static int calibrate;		/* actually, this is just how longer the
				 * the user has to wait compared to a normal
				 * round */

static void burn_cpu(void){
    int i = rounds;

    while(i--);
}

/*!\brief calibrate for a given number of rounds
 */
static void do_calibrate(void){
    int i;
    l4_cpu_time_t start, diff;

    printf("calibrating....\n");

    start = l4_rdtsc();
    for (i = 0; i < calibrate; i++) {
	burn_cpu();
    }
    diff = l4_rdtsc() - start;

    cycles_per_round = (double)diff / (double)(rounds * calibrate);

    /* our printf has no floating-point support */
    printf("%d.%03u cycles per round\n",
	   (int)cycles_per_round,
	   ((int)(cycles_per_round*1000))%1000);
}


static void do_work(void){
    l4_cpu_time_t start, diff;
    double load=0., load2=0., load3=0.;
    double cycles_per_run = cycles_per_round * rounds;

    rmgr_init();

    while (1){
	start = l4_rdtsc();
	burn_cpu();
	diff = l4_rdtsc() - start;

	/* load is the actual load, in used cycles per cycles */
	load = 100.*(1.- cycles_per_run / (double)diff);
	load2 = load2*.5 + load*.5;
	load3 = load3*.9 + load*.1;
	
	printf("%10lu/%10lu*1000 cycles, load %2d.%02d%% %2d.%02d%% %2d.%02d%%\n",
	       (unsigned long)(diff/1000),
	       (unsigned long)(cycles_per_run/1000),
	       (int)(load), ((int)(load*100))%100,
	       (int)(load2), ((int)(load2*100))%100,
	       (int)(load3), ((int)(load3*100))%100);
    }
}

int main(int argc, const char**argv){
    rmgr_init();

    if(parse_cmdline(&argc, &argv,
		     'r', "rounds", "number of loop-iterations per measurement",
		     PARSE_CMD_INT, 1000*1000*1000, &rounds,
		     'm', "calmult", "do calibration with given # or tests",
		     PARSE_CMD_INT, 0, &calibrate,
		     'c', "cycles10", "number of cycles per round * 10, if you know this",
		     PARSE_CMD_INT, 15, &cycles_per_round_int,
		     0)) exit(1);

    cycles_per_round = cycles_per_round_int / 10.;
    if(calibrate) do_calibrate();
    
    rmgr_set_prio(l4_myself(),prio);

    do_work();
    return 0;
}
