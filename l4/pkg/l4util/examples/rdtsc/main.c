
#include <stdio.h>
#include <l4/util/rdtsc.h>

int
main(int argc, const char *argv[])
{
  static l4_cpu_time_t tsc[] = { 10, 100, 1000, 10000, 100000, 1000000 };
  static int modes[]         = { L4_TSC_INIT_CALIBRATE, L4_TSC_INIT_KERNEL };
  static const char * const modes_names[] = 
              { "Calibrating timer", "Determining scalers from kernel" };
  int i, m;

  for (m=0; m<sizeof(modes)/sizeof(modes[0]); m++)
    {
      printf("%s ... ", modes_names[m]);
      if (l4_tsc_init(modes[m]))
	{
	  printf("done.\n"
	         "(tsc_to_ns=%08x, tsc_to_us=%08x, ns_to_tsc=%08x).\n",
	      l4_scaler_tsc_to_ns, l4_scaler_tsc_to_us, l4_scaler_ns_to_tsc);
	  for (i=0; i<sizeof(tsc)/sizeof(tsc[0]); i++)
	    {
	      printf("%8lld cycles = %8lldns = %5lldµs\n",
		  tsc[i], l4_tsc_to_ns(tsc[i]), l4_tsc_to_us(tsc[i]));
	    }
	  printf("(CPU runs at %d.%03dMHz)\n",
	      l4_get_hz() / 1000000, (l4_get_hz() % 1000000) / 1000);
	}
      else
	printf("failed.\n");
      putchar('\n');
    }

  return 0;
}
