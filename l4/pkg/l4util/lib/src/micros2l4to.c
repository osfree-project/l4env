/* 
 * $Id$
 */

/*****************************************************************************
 * libl4util/src/micros2l4to.c                                               *
 * calculate L4 timeout                                                      *
 *****************************************************************************/

/* L4 includes */
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>

unsigned long flz(unsigned long word);

__inline__ unsigned long flz(unsigned long word)
{
  __asm__("bsrl %1,%0"
	  :"=r" (word)
	  :"r" (~word));
  return word;
}

int micros2l4to(int mus, int *to_e, int *to_m)
{
  if (mus <= 0)
    {
      *to_e = 1;
      *to_m = 0;
    }
  else
    {
      *to_e = 14 - (flz(~(mus / 256))) / 2;
      *to_m = mus / (1UL << (2 * (15 - *to_e)));
      
      if ((*to_e < 0) || (*to_e > 15) || (*to_m < 0) || (*to_m > 255))
        {
	  outstring("micros2l4to: mus = ");
	  outdec(mus);
	  outstring(", to_e = ");
	  outdec(*to_e);
	  outstring(", to_m = ");
	  outdec(*to_m);
	  outstring("\n\r");
	  enter_kdebug("micros2l4to");

	  return -1;
        }
    }
  return 0;
}
