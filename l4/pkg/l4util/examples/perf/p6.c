/* $Id$
   
   Example for performance measurement counter usage at Intel-P6.
   This does not run at 386, 486 or 586 processors. For Pentium you
   have to use other mechanisms for performance counter access.
*/

#include <stdio.h>
#include <string.h>
#include <l4/rmgr/librmgr.h>
#include <l4/util/util.h>
#include <l4/util/perform.h>
#include <l4/sys/kdebug.h>

unsigned char data_array[1024*1024];

int main(int argc, char **argv){
  unsigned long long v0=-1, v1=-1;
  
  rmgr_init();
  printf("perf_p6");
  
  printf("selecting L2-Requests for user+kernel");
  l4_i686_select_perfctr_event(	0,		// the counter
  				P6_L2_RQSTS |	// the event: cache requests
  				P6CNT_U | 	// we count in user level
  				P6CNT_K |	// we count in kernel level
  				P6_UNIT_MESI |	// all MESI states
  				P6CNT_EN);	// and we enable the counters
  printf("selected");
  l4_i586_rdmsr(MSR_P6_PERFCTR0, &v0);
  printf("Counter is %lu now\n", (long)v0);
  
  memset(data_array, 3, sizeof(data_array));
  
  /* we would like to use rdpmc, but in current L4 not allowed in cr4 */
  //l4_i686_rdpmc(0, &v1);
  /* so we have to use the rdmsr which traps into kernel */
  l4_i586_rdmsr(MSR_P6_PERFCTR0, &v1);
  printf("After a while, counter is %lu.%lu now\n", (long)(v1>>32), (long)v1);
  
  printf("falling asleep...");
  enter_kdebug(".");  
  while(1){
    l4_sleep(1000);
    l4_i586_rdmsr(MSR_P6_PERFCTR0, &v1);
    printf("After a while, counter is %lu.%lu now\n", (long)(v1>>32), (long)v1);
  }
}
