#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kernel.h>
#include <l4/sigma0/sigma0.h>
#include <l4/sigma0/kip.h>
#include <l4/rmgr/librmgr.h>
#include <l4/rmgr/proto.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>
#include <l4/util/irq.h>
#include <l4/util/apic.h>
#include <l4/util/cpu.h>

#include <l4/util/perform.h>
#include <l4/util/rdtsc.h>

static l4_kernel_info_t * l4_kernel_info;
static long long nmi_perf = 0;

/* Map apic memory mapped i/o registers */
static void
apic_map_iopage(l4_addr_t map_addr)
{
  l4_addr_t page_addr;
  l4_threadid_t rmgr_pager_id;

  apic_done();
 
  page_addr = l4_trunc_superpage(APIC_PHYS_BASE);
  rmgr_pager_id = rmgr_id;
  rmgr_pager_id.id.lthread = RMGR_LTHREAD_PAGER;

  for (;;)
    {
      switch (l4sigma0_map_iomem(rmgr_pager_id, page_addr, map_addr,
				 L4_SUPERPAGESIZE, 0))
	{
	case -2:
	  printf("Can't map APIC page (IPC error)\n");;
	  exit(-1);

	case -3:
	  /* apic page is not mapped, so wait a little bit and try again */
	  printf("Can't map APIC page, trying again...\n");
	  l4_sleep(100);
	  continue;
	}
      break;
    }

  /* tell apic library where the APIC is */
  apic_init(map_addr);
}

/* Unmap apic page so other L4 apps may use the APIC later. Device
 * superpages are exclusive mapped to one L4 task (see L4 Sigma0 protocol) */
static void
apic_unmap_iopage(void)
{
  if (!apic_map_base)
    {
      printf("Can't unmap APIC page");
      exit(-1);
    }

  l4_fpage_unmap(l4_fpage(apic_map_base & L4_SUPERPAGEMASK,
			  L4_LOG2_SUPERPAGESIZE, 0, 0),
		 L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);

  apic_done();

  rmgr_free_page((APIC_PHYS_BASE & L4_SUPERPAGEMASK) - 0x40000000);
}

/* Checks if we get IRQ's yet (after activating APIC). Check if the
 * kernel timer tick changes. */
static unsigned long
check_getirqs(void)
{
  l4_uint32_t clock = (l4_uint32_t)l4_kernel_info->clock;
  l4_uint32_t count;

  for (count=0x20000000; count; count--)
    if (clock != (l4_uint32_t)l4_kernel_info->clock)
      break;

  return count;
}

/* Some newer CPUs (AMD K7, PIII) have disabled the local APIC. Try to 
 * activate it. Check if it is really working. */
static int
try_to_activate_apic(void)
{
  printf("Local APIC disabled? Trying to activate:");
  apic_activate_by_msr();
  if (apic_test_present())
    {
      printf(" Present");
      if (apic_check_working())
	{
	  printf(", Working");
	  apic_activate_by_io();
	  if (check_getirqs())
	    {
	      printf(".\n");
	      return 1;
	    }

	  printf(", No IRQs!\n");
	  return 0;
	}

      printf(", Not working!\n");
      return 0;
    }

  printf(" Not present!\n");
  return 0;
}

/* Initialize local APIC.
 * Returns 1 on success */
static int
init_apic(void)
{
  int have_apic = 1;

  /* map kernel info page */
  l4_kernel_info = l4sigma0_kip_map(L4_INVALID_ID);

  /* map APIC memory mapped i/o registers */
  apic_map_iopage(APIC_MAP_BASE);

  if (   !apic_test_present()
      || !apic_check_working())
    have_apic = try_to_activate_apic();

  if (have_apic)
    {
      apic_soft_enable();
      printf("Local APIC version 0x%02lx, activating watchdog...\n",
	  GET_APIC_VERSION(apic_read(APIC_LVR)));
    }

  return have_apic;
}

/* Reset performance counter 1 */
static void inline
set_nmi_counter_local(void)
{
#ifdef CPU_P6
  l4_i586_wrmsr(MSR_P6_PERFCTR1, &nmi_perf);
#else
  l4_i586_wrmsr(MSR_K7_PERFCTR1, &nmi_perf);
#endif
}

int
main(int argc, char **argv)
{
  unsigned long long val;
  unsigned long hz;
  unsigned long to;
  unsigned char vendor_string[13];
  unsigned long dummy;
  unsigned long version;
  unsigned long family;

  /* Set to high priority so no other L4 task get scheduled until
   * we are going into the wait loop */
  if (rmgr_set_prio(l4_myself(), 0xff))
    printf("Can't set priority of myself to 255!!\n");

  if (!l4util_cpu_has_cpuid())
    {
      printf("CPUID not support by CPU\n");
      exit(1);
    }

  vendor_string[12] = 0;
  l4util_cpu_cpuid(0, &dummy,
                   (unsigned long *)vendor_string,
                   (unsigned long *)(vendor_string + 8),
                   (unsigned long *)(vendor_string + 4));

  l4util_cpu_cpuid(1, &version, &dummy, &dummy, &dummy);
  family = (version >> 8) & 0xf;

#ifdef CPU_P6
  if ((memcmp(vendor_string, "GenuineIntel", 12)) || (family < 6))
    {
      printf("CPU must be at least an Intel P6!\n");
      return -1;
    }
#else
  if ((memcmp(vendor_string, "AuthenticAMD", 12)) || (family < 6))
    {
      printf("CPU must be at least an AMD K7!\n");
      return -1;
    }
#endif

  if (!rmgr_init())
    {
      printf("Error initializing rmgr\n");
      return -1;
    }

  if (!init_apic())
    {
      printf("Error initializing local APIC\n");
      return -1;
    }

  /* calibrate delay loop */
  l4util_cli();
  l4_calibrate_tsc();
  l4util_sti();

  if ((hz = l4_get_hz()))
    {
      unsigned mhz, khz;
      mhz = hz / 1000000;
      khz = (hz - mhz * 1000000) / 1000;
      printf("CPU frequency is %d.%03d MHz\n", mhz, khz);
    }

  to = 4; /* default timeout is 10s */

  /* set nmi timer tick */
  nmi_perf  = ((long long)((hz >> 16) * to));
  nmi_perf <<= 16;

  /* The maximum value a performance counter register can be written to
   * is 0x7ffffffff. The 31st bit is extracted to the bits 32-39 (see
   * "IA-32 Intel Architecture Software Developer's Manual. Volume 3:
   * Programming Guide" section 14.10.2: PerfCtr0 and PerfCtr1 MSRs */
  if (nmi_perf > 0x7fffffff)
    nmi_perf = 0x7fffffff;

  /* negate the value because the interrupt is generated when the counter
   * passes 0 */
  nmi_perf = -nmi_perf;

  /* disable performance counters 0, 1 for all NMI changes */
  val = 0;
#ifdef CPU_P6
  l4_i586_wrmsr(MSR_P6_EVNTSEL0, &val);
  l4_i586_wrmsr(MSR_P6_EVNTSEL1, &val);
  l4_i586_wrmsr(MSR_P6_PERFCTR0, &val);
  l4_i586_wrmsr(MSR_P6_PERFCTR1, &val);
#else
  l4_i586_wrmsr(MSR_K7_EVNTSEL0, &val);
  l4_i586_wrmsr(MSR_K7_EVNTSEL1, &val);
  l4_i586_wrmsr(MSR_K7_PERFCTR0, &val);
  l4_i586_wrmsr(MSR_K7_PERFCTR1, &val);
#endif

  /* enable int, os, user mode, events: cpu clocks not halted */
#ifdef CPU_P6
  val = P6CNT_IE | P6CNT_K | P6CNT_U | P6_CPU_CLK_UNHALTED;
  l4_i586_wrmsr(MSR_P6_EVNTSEL1, &val);
#else
  val = K7CNT_IE | K7CNT_K | K7CNT_U | 0xd0;
  l4_i586_wrmsr(MSR_K7_EVNTSEL1, &val);
#endif

  /* reset counter */
  set_nmi_counter_local();

  /* set NMI mode for performance counter interrrupt */
  apic_write(APIC_LVTPC, 0x400);

  /* unmap APIC flexpage so other server can use it */
  apic_unmap_iopage();

  /* enable both performance counters */
#ifdef CPU_P6
  val = P6CNT_EN;
  l4_i586_wrmsr(MSR_P6_EVNTSEL0, &val);
#else
  val |= K7CNT_EN;
  l4_i586_wrmsr(MSR_K7_EVNTSEL1, &val);
#endif

#ifdef DEMO
  printf("Going into cli'd sleep ...\n");
  l4util_cli();
  for (;;)
      ;
#endif

  printf("watchdog enabled");

  for (;;)
    {
      /* XXX we have to check here if one second is too long. */
      l4_sleep(1000);
      set_nmi_counter_local();
    }

  return 0;
}
