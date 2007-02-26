
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <flux/x86/cpuid.h>

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/rmgr/librmgr.h>
#include <l4/rmgr/proto.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>
#include <l4/util/apic.h>
#include <l4/sys/kernel.h>

#include <l4/util/perform.h>
#include <l4/util/rdtsc.h>

static l4_kernel_info_t *l4_kernel_info = (l4_kernel_info_t *)0x1000;

static long long nmi_perf = 0;

/* Get kernel info page. We need it for the kernel timer tick. */
static void
map_kernel_info_page(void)
{
  int error;
  l4_msgdope_t result;
  l4_snd_fpage_t fpage;
  l4_threadid_t rmgr_pager_id;

  rmgr_pager_id = rmgr_id;
  rmgr_pager_id.id.lthread = RMGR_LTHREAD_PAGER;

  error = l4_i386_ipc_call(rmgr_pager_id, L4_IPC_SHORT_MSG, 1, 1,
                           L4_IPC_MAPMSG((l4_addr_t)l4_kernel_info,
                                         L4_LOG2_PAGESIZE),
                           &fpage.snd_base, &fpage.fpage.fpage,
                           L4_IPC_NEVER, &result);
  if (error)
    {
      printf("Can't map KI page");
      exit(-1);
    }
  else if (!l4_ipc_fpage_received(result))
    {
      printf("CPUfreq: KI page not mapped");
      exit(-1);
    }
}

/* Map apic memory mapped i/o registers */
static void
apic_map_iopage(unsigned long map_addr)
{
  int error;
  l4_umword_t dummy;
  l4_addr_t page_addr;
  l4_msgdope_t result;
  l4_threadid_t rmgr_pager_id;

  apic_done();
    
  page_addr = (APIC_PHYS_BASE & L4_SUPERPAGEMASK) - 0x40000000;

  rmgr_pager_id = rmgr_id;
  rmgr_pager_id.id.lthread = RMGR_LTHREAD_PAGER;

  for (;;)
    {
      error = l4_i386_ipc_call(rmgr_pager_id, L4_IPC_SHORT_MSG, page_addr, 0,
			       L4_IPC_MAPMSG(map_addr,L4_LOG2_SUPERPAGESIZE),
			       &dummy, &dummy,
			       L4_IPC_NEVER, &result);
      if (error)
	{
	  printf("Can't map APIC page (error %02x)", error);
	  exit(-1);
	}
      else if (!l4_ipc_fpage_received(result))
	{
	  /* apic page is not mapped, so wait a little bit and try again */
	  printf("Can't map APIC page, trying again...\n");
	  l4_sleep(100);
	  continue;
	}
      else
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
  unsigned long res;
  asm volatile
	("movl   (%%ebx), %%ecx\n\t"
	 "0:cmpl (%%ebx), %%ecx\n\t"
	 "jne    1f\n\t"
	 "dec    %%eax\n\t"
	 "jne    0b\n\t"
	 "1:"

	:"=a" (res)
	:"a" (0x20000000), /* hope, that's sufficient :-) */
	 "b" (&l4_kernel_info->clock)
	:"ecx");
  return res;
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
  map_kernel_info_page();
  
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
  struct cpu_info cpu;
  unsigned long hz;
  unsigned long to;

  /* Set to high priority so no other L4 task get scheduled until
   * we are going into the wait loop */
  if (rmgr_set_prio(l4_myself(), 0xff))
    printf("Can't set priority of myself to 255!!\n");

  /* Check for CPU type */
  cpuid(&cpu);

#ifdef CPU_P6
  if ((memcmp(cpu.vendor_id, "GenuineIntel", 12)) || (cpu.family < 6))
    {
      printf("CPU must be at least an Intel P6!\n");
      return -1;
    }
#else
  if ((memcmp(cpu.vendor_id, "AuthenticAMD", 12)) || (cpu.family < 6))
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
  asm volatile("cli");
  l4_calibrate_tsc();
  asm volatile("sti");

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
  asm volatile ("cli");
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

