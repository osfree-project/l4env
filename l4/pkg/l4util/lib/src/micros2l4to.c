/* 
 * $Id$
 */

/*****************************************************************************
 * libl4util/src/micros2l4to.c                                               *
 * calculate L4 timeout                                                      *
 *****************************************************************************/

#include <stdio.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>
#include <l4/util/bitops.h>

int
l4util_micros2l4to(int mus, int *to_m, int *to_e)
{
  if (mus == 0)
    {
      *to_e = 1;
      *to_m = 0;
    }
  else if (mus == -1)
    {
      *to_e = 0;
      *to_m = 0;
    }
  else
    {
      *to_e = 15 - (l4util_log2(mus>>7)+1) / 2;
      *to_m = mus / (1UL << (2 * (15 - *to_e)));
      
      if ((*to_e < 0) || (*to_e > 15) || (*to_m < 0) || (*to_m > 255))
        {
	  printf("l4util_micros2l4to(): "
	         "invalid timeout %d, using max. values\n", mus);
	  *to_e = 0;
	  *to_m = 255;
        }
    }
  return 0;
}

int
l4util_l4to2micros(int to_m, int to_e)
{
  if (to_e<0 || to_e>15 || to_m<0 || to_m>255)
    return -1;

  if (to_e == 0)
    return -1;

  if (to_m == 0)
    return 0;

  /* value: 4^(15-e) * m */
  return (1<<(2*(15-to_e)))*to_m;
}
