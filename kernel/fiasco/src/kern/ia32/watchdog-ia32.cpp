INTERFACE:

#include "types.h"

class Watchdog
{
public:
  static void (*touch)(void);
  static void (*enable)(void);
  static void (*disable)(void);
  static void init();
  
private:
  Watchdog();                        // default constructors are undefined
  Watchdog(const Watchdog&);

  static void init_perf_p4();
  static void init_perf_p6();
  static void init_perf_k7();

  static void (*init_perf)(void);
  static Unsigned64 nmi_perf;

  typedef struct
    {
      unsigned active:1;
      unsigned user_active:1;
      unsigned no_user_control:1;
    } Watchdog_flags_t;

  static Watchdog_flags_t flags;
};


IMPLEMENTATION[ia32]:

#include <cstdio>
#include <cstdlib>

#include "apic.h"
#include "config.h"
#include "cpu.h"
#include "initcalls.h"

#define WATCHDOG_TIMEOUT_S	2

#define MSR_P6_EVNTSEL0		0x186
#define MSR_P6_EVNTSEL1		0x187
#define MSR_P6_PERFCTR0		0xc1
#define MSR_P6_PERFCTR1		0xc2

#define MSR_P4_MISC_ENABLE	0x1A0
#define MSR_P4_MISC_ENABLE_PERF_AVAIL	(1<<7)
#define MSR_P4_MISC_ENABLE_PEBS_UNAVAIL	(1<<12)
#define MSR_P4_PERFCTR0		0x300
#define MSR_P4_CCCR0		0x360

#define MSR_K7_EVNTSEL0		0xC0010000
#define MSR_K7_EVNTSEL1		0xC0010001
#define MSR_K7_EVNTSEL2		0xC0010002
#define MSR_K7_EVNTSEL3		0xC0010003
#define MSR_K7_PERFCTR0		0xC0010004
#define MSR_K7_PERFCTR1		0xC0010005
#define MSR_K7_PERFCTR2		0xC0010006
#define MSR_K7_PERFCTR3		0xC0010007

#define MSR_P4_IQ_COUNTER0	0x30C

#define MSR_P4_CRU_ESCR0	0x3B8
#define P4_ESCR_EVENT_SELECT(N)	((N)<<25)
#define P4_ESCR_OS		(1<<3)
#define P4_ESCR_USR		(1<<2)

#define MSR_P4_IQ_CCCR0		0x36C
#define P4_CCCR_OVF_PMI		(1<<26)
#define P4_CCCR_THRESHOLD(N)	((N)<<20)
#define P4_CCCR_COMPLEMENT	(1<<19)
#define P4_CCCR_COMPARE		(1<<18)
#define P4_CCCR_REQUIRED	(3<<16)
#define P4_CCCR_ESCR_SELECT(N)	((N)<<13)
#define P4_CCCR_ENABLE		(1<<12)


void (*Watchdog::init_perf)(void) = do_nothing;
void (*Watchdog::touch)(void)     = do_nothing;
void (*Watchdog::enable)(void)    = do_nothing;
void (*Watchdog::disable)(void)   = do_nothing;

Unsigned64 Watchdog::nmi_perf;
Watchdog::Watchdog_flags_t Watchdog::flags = 
(Watchdog::Watchdog_flags_t) { active:0, user_active:0, no_user_control:1 };

static inline
void
clear_msr_range(unsigned int base, unsigned int n)
{
  for (unsigned i=0; i<n; i++)
    Cpu::wrmsr(0, base+i);
}

static
void
do_nothing()
{
}


// ===== P6/PII/PIII =====
IMPLEMENT FIASCO_INIT
void
Watchdog::init_perf_p6()
{
  Unsigned64 msr;
  
  msr = 0x100000	// Int enable: enable interrupt on overflow
      | 0x020000	// Monitor kernel-level events
      | 0x010000	// Monitor user-level events
      |     0x79;	// #clocks CPU is not halted
  Cpu::wrmsr(msr, MSR_P6_EVNTSEL1);
  
  msr = 0x400000;	// enable both!! counters
  Cpu::wrmsr(msr, MSR_P6_EVNTSEL0);
}

static
void
Watchdog::enable_perf_nmi_p6()
{
  if (flags.active && flags.user_active)
    {
      Unsigned64 msr;
      
      Cpu::wrmsr(nmi_perf, MSR_P6_PERFCTR1);
      msr = Cpu::rdmsr(MSR_P6_EVNTSEL1);
      msr |= 0x100000;	// Int enable: enable interrupt on overflow
      Cpu::wrmsr(msr, MSR_P6_EVNTSEL1);
    }
}

static
void
Watchdog::disable_perf_nmi_p6()
{
  if (flags.active)
    {
      Unsigned64 msr;
      
      msr = Cpu::rdmsr(MSR_P6_EVNTSEL1);
      msr &= ~0x100000;	// Int enable: enable interrupt on overflow
      Cpu::wrmsr(msr, MSR_P6_EVNTSEL1);
    }
}

static
void
Watchdog::touch_perf_p6()
{
  if (flags.active && flags.user_active && flags.no_user_control)
    Cpu::wrmsr(nmi_perf, MSR_P6_PERFCTR1);
}


// ===== PIV =====
IMPLEMENT FIASCO_INIT
void
Watchdog::init_perf_p4()
{
  unsigned int misc_enable;
  Unsigned64 msr;
  
  misc_enable = Cpu::rdmsr(MSR_P4_MISC_ENABLE);
 
  // disable precise event based sampling
  if (!(misc_enable & MSR_P4_MISC_ENABLE_PEBS_UNAVAIL))
    clear_msr_range(0x3F1, 2);
  
  // ensure sane state of performance counter registers
  clear_msr_range(0x3A0, 31);
  clear_msr_range(0x3C0, 6);
  clear_msr_range(0x3C8, 6);
  clear_msr_range(0x3E0, 2);
  clear_msr_range(MSR_P4_CCCR0, 18);
  clear_msr_range(MSR_P4_PERFCTR0, 18);

  // Set up IQ_COUNTER0 to behave like a clock, by having IQ_CCCR0 filter
  // CRU_ESCR0 (with any non-null event selector) through a complemented
  // max threshold. [IA32-Vol3, Section 14.9.9]
  msr = P4_ESCR_EVENT_SELECT(0x3F) // don't care -- non-null
      | P4_ESCR_OS		// Monitor kernel-level events
      | P4_ESCR_USR;		// Monitor user-level events

  Cpu::wrmsr(msr, MSR_P4_CRU_ESCR0);

  msr = P4_CCCR_OVF_PMI		// performance monitor interrupt on overflow
      | P4_CCCR_THRESHOLD(15)	// threshold is met every cycle, count anything
      | P4_CCCR_COMPLEMENT	// count events less or equal threshold
      | P4_CCCR_COMPARE		// enable filtering (complement,threshold,edge)
      | P4_CCCR_REQUIRED	// must be set
      | P4_CCCR_ESCR_SELECT(4);	// select ESCR to select events to be counted
  
  Cpu::wrmsr(msr, MSR_P4_IQ_CCCR0);

  msr |= P4_CCCR_ENABLE;
  
  Cpu::wrmsr(msr, MSR_P4_IQ_CCCR0);
}

static
void
Watchdog::enable_perf_nmi_p4()
{
  if (flags.active && flags.user_active)
    {
      Unsigned64 msr;
      
      // re-enable counter
      msr = P4_CCCR_OVF_PMI
	  | P4_CCCR_THRESHOLD(15)
	  | P4_CCCR_COMPLEMENT
	  | P4_CCCR_COMPARE
	  | P4_CCCR_REQUIRED
	  | P4_CCCR_ESCR_SELECT(4)
	  | P4_CCCR_ENABLE;
      Cpu::wrmsr(msr, MSR_P4_IQ_CCCR0);
      
      Cpu::wrmsr(nmi_perf, MSR_P4_IQ_COUNTER0);
    }
}

static
void
Watchdog::disable_perf_nmi_p4()
{
  if (flags.active)
    {
      Unsigned64 msr;
      
      msr = Cpu::rdmsr(MSR_P4_IQ_CCCR0);
      msr &= ~P4_CCCR_OVF_PMI;
      Cpu::wrmsr(msr, MSR_P4_IQ_CCCR0);
    }
}

static
void
Watchdog::touch_perf_p4()
{
  if (flags.active && flags.user_active && flags.no_user_control)
    Cpu::wrmsr(nmi_perf, MSR_P4_IQ_COUNTER0);
}


// ===== K7 =====
IMPLEMENT FIASCO_INIT
void
Watchdog::init_perf_k7()
{
  Unsigned64 msr;
  
  msr = 0x100000	// Int enable: enable interrupt on overflow
      | 0x020000	// Monitor kernel-level events
      | 0x010000	// Monitor user-level events
      |     0x76;	// no idea what this is but it seems to work
  Cpu::wrmsr(msr, MSR_K7_EVNTSEL1);
  
  msr |= 0x400000;	// enable
  Cpu::wrmsr(msr, MSR_K7_EVNTSEL1);
}

static
void
Watchdog::enable_perf_nmi_k7()
{
  if (flags.active && flags.user_active)
    {
      Unsigned64 msr;
      
      Cpu::wrmsr(nmi_perf, MSR_K7_PERFCTR1);
      msr = Cpu::rdmsr(MSR_K7_EVNTSEL1);
      msr |= 0x100000;	// Int enable: enable interrupt on overflow
      Cpu::wrmsr(msr, MSR_K7_EVNTSEL1);
    }
}

static
void
Watchdog::disable_perf_nmi_k7()
{
  if (flags.active)
    {
      Unsigned64 msr;
      
      msr = Cpu::rdmsr(MSR_K7_EVNTSEL1);
      msr &= ~0x100000;	// Int enable: enable interrupt on overflow
      Cpu::wrmsr(msr, MSR_K7_EVNTSEL1);
    }
}

static
void
Watchdog::touch_perf_k7()
{
  if (flags.active && flags.user_active && flags.no_user_control)
    Cpu::wrmsr(nmi_perf, MSR_K7_PERFCTR1);
}


// user enables Watchdog
PUBLIC static inline
void
Watchdog::user_enable()
{
  flags.user_active = 1;
  enable();
}

// user disables Watchdog
PUBLIC static inline
void
Watchdog::user_disable()
{
  flags.user_active = 0;
  disable();
}

// user takes over control of Watchdog
PUBLIC static inline
void
Watchdog::user_takeover_control()
{
  flags.no_user_control = 0;
}

// user gives back control of Watchdog
PUBLIC static inline
void
Watchdog::user_giveback_control()
{
  flags.no_user_control = 0;
}

IMPLEMENT FIASCO_INIT
void
Watchdog::init()
{
  unsigned long hz;

  if (   !Apic::is_present()
      || !(hz = Cpu::hz())
      || !Apic::have_pcint())
    {
      Config::watchdog = false;
      return;
    }

  if (Apic::cpu_type() == Apic::APIC_P6)
    {
      init_perf = init_perf_p6;
      touch     = touch_perf_p6;
      enable    = enable_perf_nmi_p6;
      disable   = disable_perf_nmi_p6;
    }
  else if (Apic::cpu_type() == Apic::APIC_P4)
    {
      unsigned misc_enable;
      
      misc_enable = Cpu::rdmsr(MSR_P4_MISC_ENABLE);
      if (!(misc_enable & MSR_P4_MISC_ENABLE_PERF_AVAIL))
	{
	  // P4 performance counter disabled
	  Config::watchdog = false;
	  return;
	}
      
      init_perf = init_perf_p4;
      touch     = touch_perf_p4;
      enable    = enable_perf_nmi_p4;
      disable   = disable_perf_nmi_p4;
    }
  else if (Apic::cpu_type() == Apic::APIC_K7)
    {
      init_perf = init_perf_k7;
      touch     = touch_perf_k7;
      enable    = enable_perf_nmi_k7;
      disable   = disable_perf_nmi_k7;
    }
  else
    {
      Config::watchdog = false;
      return;
    }

  nmi_perf = ((Unsigned64)((hz >> 16) * WATCHDOG_TIMEOUT_S)) << 16;

  // The maximum value a performance counter register can be written to
  // is 0x7ffffffff. The 31st bit is extracted to the bits 32-39 (see
  // "IA-32 Intel Architecture Software Developer's Manual. Volume 3:
  // Programming Guide" section 14.10.2: PerfCtr0 and PerfCtr1 MSRs
  if (nmi_perf > 0x7fffffff)
    nmi_perf = 0x7fffffff;

  // negate the value because the interrupt is generated when the increasing
  // counter passes 0
  nmi_perf = -nmi_perf;

  // reset counter
  touch();

  // attach performance counter interrupt tom NMI
  Apic::set_perf_nmi();
  
  // start counter
  init_perf();

  flags.active = 1;
  flags.user_active = 1;
  flags.no_user_control = 1;

  printf("Watchdog initialized\n");
}
