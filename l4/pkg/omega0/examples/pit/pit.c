#include <l4/sys/compiler.h>
#include <l4/util/port_io.h>
#include "pit.h"

void pit_set_freq(int freq){
  // set frequency for timer 0 at 8284-PIT
  int val;
  
  if(!freq)freq=1;
  val = 1193180/freq;
  if(val>=65536) val = 0;
  
  l4util_out8(0x34, 0x43);	// write lower and higher byte of counter 0,
				// mode 2 (periodic impulses), no bcd
  l4util_out8(val&0xff, 0x40);
  l4util_out8(val>>8, 0x40);
}
