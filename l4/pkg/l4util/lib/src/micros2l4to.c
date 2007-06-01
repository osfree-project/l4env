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

l4_timeout_s
l4util_micros2l4to(int mus)
{
  l4_timeout_s t;
  if (mus == 0)
    t.t = 0x0400;
  else if (mus == -1)
    t.t = 0;
  else
    {
      int e = l4util_log2(mus) - 7;
      unsigned m;

      if (e < 0) e = 0;
      m = mus / (1UL << e);

      if ((e < 0) || (e > 31) || (m < 0) || (m > 1023))
        {
	  printf("l4util_micros2l4to(): "
	         "invalid timeout %d, using max. values\n", mus);
	  e = 0;
	  m = 1023;
        }
      t.t = (e << 10) | m;
    }
  return t;
}

