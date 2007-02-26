#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kernel.h>
#include <l4/rmgr/librmgr.h>
#include <l4/util/port_io.h>
#include <l4/util/irq.h>
#include <l4/util/rdtsc.h>
#include <stdio.h>

l4_uint32_t l4_scaler_tsc_to_ns;
l4_uint32_t l4_scaler_tsc_to_us;
l4_uint32_t l4_scaler_ns_to_tsc;
l4_uint32_t l4_scaler_tsc_linux;


static inline l4_uint32_t
muldiv (l4_uint32_t a, l4_uint32_t mul, l4_uint32_t div)
{
  asm ("mull %1 ; divl %2\n\t"
       :"=a" (a)
       :"d" (mul), "c" (div), "0" (a));
  return a;
}


static char kip_area[L4_PAGESIZE] __attribute__((aligned(L4_PAGESIZE)));

static void
unmap_kip(void)
{
  l4_fpage_unmap(l4_fpage((l4_addr_t)kip_area, L4_LOG2_PAGESIZE,
			  L4_FPAGE_RW, L4_FPAGE_MAP),
		 L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);
}

/*
 * Return 2^32 / (tsc clocks per usec) 
 */
l4_uint32_t
l4_calibrate_tsc (void)
{
  /*
   * First, lets try to get the info out of the kernel info page so that
   * we don't need to do port i/o. If we're unable to get the information
   * there, measure it ourselves.
   */

  {
    l4_snd_fpage_t fp;
    l4_msgdope_t dope;
    l4_kernel_info_t *kip = (l4_kernel_info_t *)kip_area;
    int e;

    if (!rmgr_init())
      printf("rmgr_init failed!\n");

    unmap_kip();

    e = l4_ipc_call(rmgr_pager_id, L4_IPC_SHORT_MSG, 1, 1,
                    L4_IPC_MAPMSG((l4_addr_t)kip, L4_LOG2_PAGESIZE),
                    &fp.snd_base, &fp.snd_base, L4_IPC_NEVER, &dope);

    if (!e && l4_ipc_fpage_received(dope))
      {
        if (kip->frequency_cpu 
	    && kip->frequency_cpu < 50000000 /* sanity check*/)
	  {
	    l4_scaler_tsc_linux = muldiv(1 << 30, 4000, kip->frequency_cpu);
	    l4_scaler_ns_to_tsc = muldiv(1 << 27, kip->frequency_cpu, 1000000);

	    /* l4_scaler_ns_to_tsc = (2^32 * Hz) / (32 * 1.000.000.000) */
	  }
	else
	  printf("CPU frequency not set in KIP, doing measurement myself.\n");

	unmap_kip();
      }
    else
      printf("Mapping of KIP failed!\n");
  }

  if (l4_scaler_tsc_linux == 0)
    {
#define CLOCK_TICK_RATE 	1193180
#define CALIBRATE_TIME  	50001
#define CALIBRATE_LATCH		(CLOCK_TICK_RATE / 20) /* 50 ms */

      /* Set the Gate high, disable speaker */
      l4util_out8 ((l4util_in8 (0x61) & ~0x02) | 0x01, 0x61);

      l4util_out8 (0xb0, 0x43);		/* binary, mode 0, LSB/MSB, Ch 2 */
      l4util_out8 (CALIBRATE_LATCH & 0xff, 0x42);	/* LSB of count */
      l4util_out8 (CALIBRATE_LATCH >> 8, 0x42);	/* MSB of count */

	{
	  l4_uint64_t tsc_start, tsc_end;
	  l4_uint32_t count, flags, dummy;

	  /* disable interrupts */
	  l4util_flags_save(&flags);
	  l4util_cli();

	  tsc_start = l4_rdtsc ();
	  count = 0;
	  do
	    {
	      count++;
	    }
	  while ((l4util_in8 (0x61) & 0x20) == 0);
	  tsc_end = l4_rdtsc ();

	  /* restore flags */
	  l4util_flags_restore(&flags);

	  /* Error: ECTCNEVERSET */
	  if (count <= 1)
	    goto bad_ctc;

	  /* 64-bit subtract - gcc just messes up with long longs */
	  tsc_end -= tsc_start;

	  /* Error: ECPUTOOFAST */
	  if (tsc_end & 0xffffffff00000000LL)
	    goto bad_ctc;

	  /* Error: ECPUTOOSLOW */
	  if ((tsc_end & 0xffffffffL) <= CALIBRATE_TIME)
	    goto bad_ctc;

	  asm ("divl %2"
	      :"=a" (l4_scaler_tsc_linux), "=d" (dummy)
	      :"r" ((l4_uint32_t)tsc_end),  "0" (0), "1" (CALIBRATE_TIME));

          l4_scaler_ns_to_tsc = muldiv(((1ULL<<32)/1000ULL),
	                               (l4_uint32_t)tsc_end,
				       CALIBRATE_TIME * (1<<5));
	}
    }

  l4_scaler_tsc_to_ns = muldiv(l4_scaler_tsc_linux, 1000, 1<<5);
  l4_scaler_tsc_to_us = muldiv(l4_scaler_tsc_linux,    1, 1<<5);

  /* l4_scaler_tsc_to_ns = (2^32 * 1.000.000.000) / (32 * Hz)
   * l4_scaler_ns_to_tsc = (2^32 * Hz) / (32 * 1.000.000.000)
   * l4_scaler_tsc_to_us = (2^32 * 1.000.000)     / (32 * Hz) */

  return l4_scaler_tsc_linux;

  /*
   * The CTC wasn't reliable: we got a hit on the very first read,
   * or the CPU was so fast/slow that the quotient wouldn't fit in
   * 32 bits..
   */
bad_ctc:
  return 0;
}

l4_uint32_t
l4_get_hz (void)
{
  if (!l4_scaler_tsc_to_ns)
    return 0;

  return (l4_uint32_t)(l4_ns_to_tsc(1000000000UL));
}

