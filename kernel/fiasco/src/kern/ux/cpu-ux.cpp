/*
 * Fiasco-UX
 * Architecture specific cpu init code
 */

IMPLEMENTATION[ux]:

#include <cstdio>
#include "initcalls.h"
#include "regdefs.h"

IMPLEMENT FIASCO_INIT
void
Cpu::init (void)
{
  identify();

  // No Sysenter Support for Fiasco-UX
  cpu_features &= ~FEAT_SEP;

  // Determine CPU frequency
  FILE *fp;
  if ((fp = fopen ("/proc/cpuinfo", "r")) != NULL)
    {
      char buffer[128];
      float val;

      while (fgets (buffer, sizeof (buffer), fp))
        if (sscanf (buffer, "cpu MHz%*[^:]: %f", &val) == 1)
          {
            cpu_frequency    = (Unsigned64)(val * 1000) * 1000;
	    scaler_tsc_to_ns = muldiv(1<<27, 1000000000,    cpu_frequency);
	    scaler_tsc_to_us = muldiv(1<<27, 1000000,       cpu_frequency);
	    scaler_ns_to_tsc = muldiv(1<<27, cpu_frequency, 1000000000);
            break;
          }

      fclose (fp);
    }
}

PUBLIC inline
static void
Cpu::set_sysenter (void (*)(void))
{
}

