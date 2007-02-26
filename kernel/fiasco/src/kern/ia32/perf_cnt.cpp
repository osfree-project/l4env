INTERFACE:

#include "types.h"

#define FIASCO_FASTCALL	  __attribute__((regparm(3)))
#define MAX_PMC           4
#define MAX_SLOT          2

class Perf_cnt_arch
{
public:
  // basic initialization
  virtual int  init() = 0;

  // set event the counter should count
  virtual void set_pmc_event(Mword slot) = 0;

  // set counter value (needed to touch the watchdog)
  virtual void clear_pmc(Mword reg_nr);

  // enable counter (on P6, we always enable both counters!)
  virtual void start_pmc(Mword reg_nr);

  // watchdog supported by performance counter architecture?
  int have_watchdog();

  // setup counter as watchdog
  virtual void init_watchdog();

  // enable watchdog interrupt
  virtual void start_watchdog();

  // disable watchdog interrupt
  virtual void stop_watchdog();

  // reset watchdog counter to <hold_watchdog>
  void touch_watchdog();

protected:
  Mword nr_regs;
  Mword sel_reg0;
  Mword ctr_reg0;
  Mword watchdog;

  typedef struct 
    {
      char  user;	// 1=count in user mode
      char  kern;	// 1=count in kernel mode
      char  edge;	// 1=count edge / 0=count duration
      Mword pmc;	// # of performance counter
      Mword bitmask;	// counter bitmask
      Mword evnt;	// event selector
    } Event;

  static Mword pmc_watchdog;		// # perfcounter of watchdog
  static Signed64 hold_watchdog;
  static Event pmc_event[MAX_SLOT];	// index is slot number
  static char  pmc_alloc[MAX_PMC];	// index is # of perfcounter
};

class Perf_cnt
{
public:
  // basic perfcounter detection
  static void init();

  // set performance counter counting the selected event in slot #slot
  static void setup_pmc(Mword slot, 
			Mword event, Mword user, Mword kern, Mword edge);

  // return human-readable type of performance counters
  static char const *perf_type();

  // return current selected event for a slot #slot
  static int  perf_mode(Mword slot, const char **mode, const char **name,
			Mword *event, Mword *user, Mword *kern, Mword *edge);

  // watchdog supported by performance counter architecture?
  static int have_watchdog();

  // setup watchdog function with timeout in seconds
  static void setup_watchdog(Mword timeout);

  // start watchdog (enable generation of overflow interrupt)
  static void start_watchdog();

  // stop watchdog (disable generation of overflow interrupt)
  static void stop_watchdog();

  // fresh up the counter
  static void touch_watchdog();

  // split event into event selector and unit mask (depending on perftype)
  static void split_event(Mword event, Mword *evntsel, Mword *unit_mask);

  // combine event from selector and unit mask
  static void combine_event(Mword evntsel, Mword unit_mask, Mword *event);

  enum Unit_mask_type
    { None, Fixed, Exclusive, Bitmask, };
  enum Perf_event_type
    { P5, P6, P4, };

  typedef Unsigned64 FIASCO_FASTCALL (*Perf_read_fn)();
  static Perf_read_fn read_pmc[MAX_SLOT];

private:
  static Perf_read_fn *read_pmc_fns;
  static Perf_read_fn read_pmc_fn[MAX_SLOT];
  static Perf_cnt_arch *pcnt;
  static char const *perf_type_str;
  static Perf_event_type perf_event_type;
};

class Perf_cnt_p5 : public Perf_cnt_arch {};
class Perf_cnt_p6 : public Perf_cnt_arch {};
class Perf_cnt_k7 : public Perf_cnt_p6   {};
class Perf_cnt_p4 : public Perf_cnt_arch {};

IMPLEMENTATION:

#include <cstring>
#include <cstdio>
#include <panic.h>
#include "cpu.h"
#include "regdefs.h"
#include "static_init.h"
#include "apic.h"

Perf_cnt::Perf_read_fn Perf_cnt::read_pmc[MAX_SLOT] =
{ dummy_read_pmc, dummy_read_pmc };
Perf_cnt::Perf_read_fn Perf_cnt::read_pmc_fn[MAX_SLOT] =
{ dummy_read_pmc, dummy_read_pmc };

Perf_cnt::Perf_read_fn *Perf_cnt::read_pmc_fns;
Perf_cnt::Perf_event_type Perf_cnt::perf_event_type;
Perf_cnt_arch *Perf_cnt::pcnt;
char const *Perf_cnt::perf_type_str = "n/a";

Mword Perf_cnt_arch::pmc_watchdog = (Mword)-1;
Signed64 Perf_cnt_arch::hold_watchdog;
Perf_cnt_arch::Event Perf_cnt_arch::pmc_event[MAX_SLOT];
char  Perf_cnt_arch::pmc_alloc[MAX_PMC];

static Perf_cnt_p5 perf_cnt_p5 __attribute__ ((init_priority(101)));
static Perf_cnt_p6 perf_cnt_p6 __attribute__ ((init_priority(101)));
static Perf_cnt_k7 perf_cnt_k7 __attribute__ ((init_priority(101)));
static Perf_cnt_p4 perf_cnt_p4 __attribute__ ((init_priority(101)));

enum {
  ALLOC_NONE            = 0,	// not used
  ALLOC_PERF            = 1,	// allocated as performance counter
  ALLOC_WATCHDOG        = 2,	// allocated for watchdog
};

enum {
  // Intel P5
  MSR_P5_CESR           = 0x11,
  MSR_P5_CTR0           = 0x12,
  MSR_P5_CTR1           = 0x13,
  P5_EVNTSEL_USER       = 0x00000080,
  P5_EVNTSEL_KERNEL     = 0x00000040,
  P5_EVNTSEL_DURATION   = 0x00000100,

  // Intel P6/PII/PIII
  MSR_P6_PERFCTR0       = 0xC1,
  MSR_P6_EVNTSEL0       = 0x186,
  P6_EVNTSEL_ENABLE     = 0x00400000,
  P6_EVNTSEL_INT	= 0x00100000,
  P6_EVNTSEL_USER       = 0x00010000,
  P6_EVNTSEL_KERNEL     = 0x00020000,
  P6_EVNTSEL_EDGE       = 0x00040000,

  // AMD K7/K8
  MSR_K7_EVNTSEL0       = 0xC0010000,
  MSR_K7_PERFCTR0       = 0xC0010004,
  K7_EVNTSEL_ENABLE     = P6_EVNTSEL_ENABLE,
  K7_EVNTSEL_INT	= P6_EVNTSEL_INT,
  K7_EVNTSEL_USER       = P6_EVNTSEL_USER,
  K7_EVNTSEL_KERNEL     = P6_EVNTSEL_KERNEL,
  K7_EVNTSEL_EDGE       = P6_EVNTSEL_EDGE,

  // Intel P4
  MSR_P4_MISC_ENABLE    = 0x1A0,
  MSR_P4_PERFCTR0       = 0x300,
  MSR_P4_IQ_COUNTER0    = 0x30C,
  MSR_P4_CCCR0          = 0x360,
  MSR_P4_CRU_ESCR0      = 0x3B8,
  P4_ESCR_USR           = (1<<2),
  P4_ESCR_OS            = (1<<3),
  MSR_P4_IQ_CCCR0       = 0x36C,
  P4_CCCR_OVF_PMI       = (1<<26),
  P4_CCCR_COMPLEMENT    = (1<<19),
  P4_CCCR_COMPARE       = (1<<18),
  P4_CCCR_REQUIRED      = (3<<16),
  P4_CCCR_ENABLE        = (1<<12),
};

#define P4_ESCR_EVENT_SELECT(N)	((N)<<25)
#define P4_CCCR_THRESHOLD(N)	((N)<<20)
#define P4_CCCR_ESCR_SELECT(N)	((N)<<13)


// -----------------------------------------------------------------------

#define PERFCTR_X86_GENERIC		0	/* any x86 with rdtsc */
#define PERFCTR_X86_INTEL_P5		1	/* no rdpmc */
#define PERFCTR_X86_INTEL_P5MMX		2
#define PERFCTR_X86_INTEL_P6		3
#define PERFCTR_X86_INTEL_PII		4
#define PERFCTR_X86_INTEL_PIII		5
#define PERFCTR_X86_AMD_K7		9
#define PERFCTR_X86_INTEL_P4		11	/* model 0 and 1 */
#define PERFCTR_X86_INTEL_P4M2		12	/* model 2 and above */
#define PERFCTR_X86_AMD_K8		13

enum perfctr_unit_mask_type {
    perfctr_um_type_fixed,	/* one fixed (required) value */
    perfctr_um_type_exclusive,	/* exactly one of N values */
    perfctr_um_type_bitmask,	/* bitwise 'or' of N power-of-2 values */
};

struct perfctr_unit_mask_value {
    unsigned int value;
    const char *description;	/* [NAME:]text */
};

struct perfctr_unit_mask {
    unsigned int default_value;
    enum perfctr_unit_mask_type type:16;
    unsigned short nvalues;
    struct perfctr_unit_mask_value values[1/*nvalues*/];
};

struct perfctr_event {
    unsigned int evntsel;
    unsigned int counters_set; /* P4 force this to be CPU-specific */
    const struct perfctr_unit_mask *unit_mask;
    const char *name;
    const char *description;
};

struct perfctr_event_set {
    unsigned int cpu_type;
    const char *event_prefix;
    const struct perfctr_event_set *include;
    unsigned int nevents;
    const struct perfctr_event *events;
};

// The following functions are only available if the perfctr module
// is linked into the kernel. If not, all symbols perfctr_* are 0. */
//
// show all events
extern "C" void perfctr_set_cputype(unsigned)
  __attribute__((weak));
extern "C" const struct perfctr_event* perfctr_lookup_event(unsigned,
							    unsigned*)
  __attribute__((weak));
extern "C" const struct perfctr_event* perfctr_index_event(unsigned)
  __attribute__((weak));
extern "C" unsigned perfctr_get_max_event(void)
  __attribute__((weak));


static inline void
clear_msr_range(Mword base, Mword n)
{
  for (Mword i=0; i<n; i++)
    Cpu::wrmsr(0, base+i);
}

//--------------------------------------------------------------------
// dummy

static FIASCO_FASTCALL
Unsigned64 dummy_read_pmc()
{
  return 0;
}

//--------------------------------------------------------------------
// Intel P5 (Pentium/Pentium MMX) has 2 performance counters. No overflow
// interrupt available. Some events are not symmtetric.
PUBLIC inline NOEXPORT
Perf_cnt_p5::Perf_cnt_p5()
  : Perf_cnt_arch(MSR_P5_CESR, MSR_P5_CTR0, 2, 0)
{
}

int
Perf_cnt_p5::init()
{
  Cpu::wrmsr(0, MSR_P5_CESR); // disable both counters
  return 1;
}

void
Perf_cnt_p5::set_pmc_event(Mword slot)
{
  Unsigned64 msr;
  Mword event;
    
  event = pmc_event[slot].evnt;
  if (pmc_event[slot].user)
    event += P5_EVNTSEL_USER;
  if (pmc_event[slot].kern)
    event += P5_EVNTSEL_KERNEL;
  if (pmc_event[slot].kern)
    event += P5_EVNTSEL_KERNEL;
  if (!pmc_event[slot].edge)
    event += P5_EVNTSEL_DURATION;

  msr = Cpu::rdmsr(MSR_P5_CESR);
  if (pmc_event[slot].pmc == 0)
    {
      msr &= 0xffff0000;
      msr |= event;
    }
  else
    {
      msr &= 0x0000ffff;
      msr |= (event << 16);
    }
  Cpu::wrmsr(event, MSR_P5_CESR);
}

static FIASCO_FASTCALL Unsigned64 p5_read_pmc_0()
{ return Cpu::rdmsr(MSR_P5_CTR0); }
static FIASCO_FASTCALL Unsigned64 p5_read_pmc_1()
{ return Cpu::rdmsr(MSR_P5_CTR1); }

static Perf_cnt::Perf_read_fn p5_read_pmc_fns[] =
{ &p5_read_pmc_0, &p5_read_pmc_1 };


//--------------------------------------------------------------------
// Intel P6 (PPro/PII/PIII) has 2 performance counters. Overflow interrupt
// is available. Some events are not symmtetric.
PUBLIC inline NOEXPORT
Perf_cnt_p6::Perf_cnt_p6()
  : Perf_cnt_arch(MSR_P6_EVNTSEL0, MSR_P6_PERFCTR0, 2, 1)
{
}

PROTECTED
Perf_cnt_p6::Perf_cnt_p6(Mword sel_reg0, Mword ctr_reg0, 
      			 Mword nr_regs, Mword watchdog)
  : Perf_cnt_arch(sel_reg0, ctr_reg0, nr_regs, watchdog)
{
}

int
Perf_cnt_p6::init()
{
  for (Mword i=0; i<nr_regs; i++)
    Cpu::wrmsr(0, sel_reg0+i);
  return 1;
}

void
Perf_cnt_p6::set_pmc_event(Mword slot)
{
  Mword event;

  event = pmc_event[slot].evnt;
  if (pmc_event[slot].user)
    event += P6_EVNTSEL_USER;
  if (pmc_event[slot].kern)
    event += P6_EVNTSEL_KERNEL;
  if (pmc_event[slot].edge)
    event += P6_EVNTSEL_EDGE;

  // select event
  Cpu::wrmsr(event, sel_reg0+pmc_event[slot].pmc);
}

void
Perf_cnt_p6::start_pmc(Mword /*reg_nr*/)
{
  Unsigned64 msr;

  msr = Cpu::rdmsr(sel_reg0);
  msr |= P6_EVNTSEL_ENABLE;  // enable both!! counters
  Cpu::wrmsr(msr, sel_reg0);
}

void
Perf_cnt_p6::init_watchdog()
{
  Unsigned64 msr;

  msr = P6_EVNTSEL_INT    // Int enable: enable interrupt on overflow
      | P6_EVNTSEL_KERNEL // Monitor kernel-level events
      | P6_EVNTSEL_USER   // Monitor user-level events
      | 0x79;             // #clocks CPU is not halted
  Cpu::wrmsr(msr, sel_reg0+pmc_watchdog);
}

void
Perf_cnt_p6::start_watchdog()
{
  Unsigned64 msr;

  msr = Cpu::rdmsr(sel_reg0+pmc_watchdog);
  msr |= P6_EVNTSEL_INT;  // Int enable: enable interrupt on overflow
  Cpu::wrmsr(msr, sel_reg0+pmc_watchdog);
}

void
Perf_cnt_p6::stop_watchdog()
{
  Unsigned64 msr;

  msr = Cpu::rdmsr(sel_reg0+pmc_watchdog);
  msr &= ~P6_EVNTSEL_INT; // Int enable: enable interrupt on overflow
  Cpu::wrmsr(msr, sel_reg0+pmc_watchdog);
}

static FIASCO_FASTCALL Unsigned64 p6_read_pmc_0() { return Cpu::rdpmc(0); }
static FIASCO_FASTCALL Unsigned64 p6_read_pmc_1() { return Cpu::rdpmc(1); }

static Perf_cnt::Perf_read_fn p6_read_pmc_fns[] =
{ &p6_read_pmc_0, &p6_read_pmc_1 };


//--------------------------------------------------------------------
// AMD K7 (Athlon, K8=Athlon64) has 4 performance counters. All events
// seem to be symmtetric. Overflow interrupts available.
PUBLIC inline NOEXPORT
Perf_cnt_k7::Perf_cnt_k7()
  : Perf_cnt_p6(MSR_K7_EVNTSEL0, MSR_K7_PERFCTR0, 4, 1)
{
}

void
Perf_cnt_k7::start_pmc(Mword reg_nr)
{
  Unsigned64 msr;

  msr = Cpu::rdmsr(sel_reg0+reg_nr);
  msr |= K7_EVNTSEL_ENABLE;
  Cpu::wrmsr(msr, sel_reg0+reg_nr);
}

void
Perf_cnt_k7::init_watchdog()
{
  Unsigned64 msr;

  msr = K7_EVNTSEL_INT    // Int enable: enable interrupt on overflow
      | K7_EVNTSEL_KERNEL // Monitor kernel-level events
      | K7_EVNTSEL_USER   // Monitor user-level events
      | 0x76;             // #clocks CPU is running
  Cpu::wrmsr(msr, sel_reg0+pmc_watchdog);
}

static FIASCO_FASTCALL Unsigned64 k7_read_pmc_0() { return Cpu::rdpmc(0); }
static FIASCO_FASTCALL Unsigned64 k7_read_pmc_1() { return Cpu::rdpmc(1); }
static FIASCO_FASTCALL Unsigned64 k7_read_pmc_2() { return Cpu::rdpmc(2); }
static FIASCO_FASTCALL Unsigned64 k7_read_pmc_3() { return Cpu::rdpmc(3); }

static Perf_cnt::Perf_read_fn k7_read_pmc_fns[] =
{ &k7_read_pmc_0, &k7_read_pmc_1, &k7_read_pmc_2, &k7_read_pmc_3 };


//--------------------------------------------------------------------
// Intel P4
PUBLIC inline NOEXPORT
Perf_cnt_p4::Perf_cnt_p4()
  : Perf_cnt_arch(MSR_P4_IQ_CCCR0, MSR_P4_IQ_COUNTER0, 1, 1)
{
}

int
Perf_cnt_p4::init()
{
  Unsigned32 misc_enable = Cpu::rdmsr(MSR_P4_MISC_ENABLE);

  if (!(misc_enable & (1<<7)))
    return 0;

  // disable precise event based sampling
  if (!(misc_enable & (1<<12)))
    clear_msr_range(0x3F1, 2);

  // ensure sane state of performance counter registers
  clear_msr_range(0x3A0, 31);
  clear_msr_range(0x3C0, 6);
  clear_msr_range(0x3C8, 6);
  clear_msr_range(0x3E0, 2);
  clear_msr_range(MSR_P4_CCCR0, 18);
  clear_msr_range(MSR_P4_PERFCTR0, 18);

  return 1;
}

void
Perf_cnt_p4::set_pmc_event(Mword /*slot*/)
{
}

void
Perf_cnt_p4::start_pmc(Mword /*reg_nr*/)
{
  Unsigned64 msr;

  msr = Cpu::rdmsr(MSR_P4_IQ_CCCR0);
  msr |= P4_CCCR_ENABLE;
  Cpu::wrmsr(msr, MSR_P4_IQ_CCCR0);
}

void
Perf_cnt_p4::init_watchdog()
{
  Unsigned64 msr;
  
  msr = P4_ESCR_EVENT_SELECT(0x3F) // don't care -- non-null
      | P4_ESCR_OS              // Monitor kernel-level events
      | P4_ESCR_USR;            // Monitor user-level events

  Cpu::wrmsr(msr, MSR_P4_CRU_ESCR0);

  msr = P4_CCCR_OVF_PMI         // performance monitor interrupt on overflow
      | P4_CCCR_THRESHOLD(15)   // threshold is met every cycle, count anything
      | P4_CCCR_COMPLEMENT      // count events less or equal threshold
      | P4_CCCR_COMPARE         // enable filtering (complement,threshold,edge)
      | P4_CCCR_REQUIRED        // must be se
      | P4_CCCR_ESCR_SELECT(4); // select ESCR to select events to be counted

  Cpu::wrmsr(msr, MSR_P4_IQ_CCCR0);
}

void
Perf_cnt_p4::start_watchdog()
{
  Unsigned64 msr;

  msr = Cpu::rdmsr(MSR_P4_IQ_CCCR0);
  msr |= P4_CCCR_OVF_PMI;       // Int enable: enable interrupt on overflow
  Cpu::wrmsr(msr, MSR_P4_IQ_CCCR0);
}

void
Perf_cnt_p4::stop_watchdog()
{
  Unsigned64 msr;

  msr = Cpu::rdmsr(MSR_P4_IQ_CCCR0);
  msr &= ~P4_CCCR_OVF_PMI;      // Int enable: enable interrupt on overflow
  Cpu::wrmsr(msr, MSR_P4_IQ_CCCR0);
}

static FIASCO_FASTCALL Unsigned64
p4_read_pmc() { return 0; }

static Perf_cnt::Perf_read_fn p4_read_pmc_fns[] =
{ &p4_read_pmc };


//--------------------------------------------------------------------
PROTECTED inline
Perf_cnt_arch::Perf_cnt_arch(Mword _sel_reg0, Mword _ctr_reg0, 
			     Mword _nr_regs, Mword _watchdog)
{
  sel_reg0 = _sel_reg0;
  ctr_reg0 = _ctr_reg0;
  nr_regs  = _nr_regs;
  watchdog = _watchdog;

  for (Mword slot=0; slot<MAX_SLOT; slot++)
    {
      pmc_event[slot].pmc  = (Mword)-1;
      pmc_event[slot].edge = 1;
    }
}

PUBLIC inline Mword
Perf_cnt_arch::watchdog_allocated()
{
  return (pmc_watchdog != (Mword)-1);
}

void
Perf_cnt_arch::alloc_watchdog()
{
  if (watchdog)
    {
      for (Mword pmc=0; pmc<nr_regs; pmc++)
	if (pmc_alloc[pmc] == ALLOC_NONE)
	  {
	    pmc_alloc[pmc] = ALLOC_WATCHDOG;
	    pmc_watchdog   = pmc;
	    return;
	  }

      panic("No performance counter available for watchdog");
    }

  panic("Watchdog not available");
}

// allocate a performance counter according to bitmask (some events depend
// on specific counters)
PRIVATE int
Perf_cnt_arch::alloc_pmc(Mword slot, Mword bitmask)
{
  // free previous allocated counter
  Mword pmc = pmc_event[slot].pmc;
  if ((pmc != (Mword)-1) && (pmc_alloc[pmc] == ALLOC_PERF))
    {
      pmc_event[slot].pmc = (Mword)-1;
      pmc_alloc[pmc]      = ALLOC_NONE;
    }

  // search counter according to bitmask
  for (pmc=0; pmc<nr_regs; pmc++)
    if ((pmc_alloc[pmc] == ALLOC_NONE) && (bitmask & (1<<pmc)))
      {
	pmc_event[slot].pmc = pmc;
	pmc_alloc[pmc]      = ALLOC_PERF;
	Perf_cnt::set_pmc_fn(slot, pmc);
	return 1;
      }

  // did not found an appropriate free counter (restricted by bitmask) so try
  // to re-assign the watchdog because the watchdog usually uses a more general
  // counter with no restrictions
  if (watchdog_allocated() && (bitmask & (1<<pmc_watchdog)))
    {
      // allocate the watchdog counter
      pmc_event[slot].pmc     = pmc_watchdog;
      pmc_alloc[pmc_watchdog] = ALLOC_PERF;
      Perf_cnt::set_pmc_fn(slot, pmc_watchdog);
      // move the watchdog to another counter
      pmc_watchdog            = (Mword)-1;
      alloc_watchdog();
      return 1;
    }

  return 0;
}

IMPLEMENT inline NOEXPORT void
Perf_cnt_arch::clear_pmc(Mword reg_nr)
{
  Cpu::wrmsr(0, ctr_reg0+reg_nr);
}

PUBLIC void
Perf_cnt_arch::perf_mode(Mword slot, const char **mode, 
			 Mword *event, Mword *user, Mword *kern, Mword *edge)
{
  const char * const mode_str[2][2][2] =
    { { { "off", "off" }, { "d.K",   "e.K"   } },
      { { "d.U", "e.U" }, { "d.K+U", "e.K+U" } } };

  *mode    = mode_str[(int)pmc_event[slot].user]
		     [(int)pmc_event[slot].kern]
		     [(int)pmc_event[slot].edge];
  *event   = pmc_event[slot].evnt;
  *user    = pmc_event[slot].user;
  *kern    = pmc_event[slot].kern;
  *edge    = pmc_event[slot].edge;
}

PUBLIC void
Perf_cnt_arch::setup_pmc(Mword slot, Mword bitmask, 
			 Mword event, Mword user, Mword kern, Mword edge)
{
  if (alloc_pmc(slot, bitmask))
    {
      pmc_event[slot].user    = user;
      pmc_event[slot].kern    = kern;
      pmc_event[slot].edge    = edge;
      pmc_event[slot].evnt    = event;
      pmc_event[slot].bitmask = bitmask;
      set_pmc_event(slot);
      clear_pmc(pmc_event[slot].pmc);
      start_pmc(pmc_event[slot].pmc);
    }
}

IMPLEMENT void
Perf_cnt_arch::start_pmc(Mword /*reg_nr*/)
{
  // nothing to do per default
}

IMPLEMENT inline int
Perf_cnt_arch::have_watchdog()
{
  return watchdog;
}

PUBLIC void
Perf_cnt_arch::setup_watchdog(Mword timeout)
{
  alloc_watchdog();
  if (watchdog_allocated())
    {
      hold_watchdog = ((Signed64)((Cpu::frequency() >> 16) * timeout)) << 16;
      // The maximum value a performance counter register can be written to
      // is 0x7ffffffff. The 31st bit is extracted to the bits 32-39 (see 
      // "IA-32 Intel Architecture Software Developer's Manual. Volume 3: 
      // Programming Guide" section 14.10.2: PerfCtr0 and PerfCtr1 MSRs.
      if (hold_watchdog > 0x7fffffff)
	hold_watchdog = 0x7fffffff;
      hold_watchdog = -hold_watchdog;
      init_watchdog();
      touch_watchdog();
      start_watchdog();
      start_pmc(pmc_watchdog);
    }
}

IMPLEMENT void
Perf_cnt_arch::init_watchdog()
{
  // no watchdog per default
}

IMPLEMENT void
Perf_cnt_arch::start_watchdog()
{
  // no watchdog per default
}

IMPLEMENT void
Perf_cnt_arch::stop_watchdog()
{
  // no watchdog per default
}

IMPLEMENT inline NOEXPORT void
Perf_cnt_arch::touch_watchdog()
{
  Cpu::wrmsr(hold_watchdog, ctr_reg0+pmc_watchdog);
}

//--------------------------------------------------------------------

STATIC_INITIALIZE_P(Perf_cnt, PERF_CNT_INIT_PRIO);

IMPLEMENT void
Perf_cnt::init()
{
  Mword have_pce     = 0;
  Mword perfctr_type = PERFCTR_X86_GENERIC;

  for (Mword i=0; i<MAX_SLOT; i++)
    read_pmc_fn[i] = dummy_read_pmc;

  if ((Cpu::features() & FEAT_TSC) && (Cpu::features() & FEAT_MSR))
    {
      if (Cpu::vendor() == Cpu::VENDOR_INTEL)
	{
	  // Intel
	  switch (Cpu::family())
	    {
	    case 5:
	      perf_event_type  = P5;
	      if (Cpu::features() & FEAT_MMX)
		{
		  perfctr_type  = PERFCTR_X86_INTEL_P5MMX;
		  perf_type_str = "P5MMX";
		  read_pmc_fns  = p6_read_pmc_fns;
		  have_pce      = 1;
		}
	      else
		{
		  perfctr_type  = PERFCTR_X86_INTEL_P5;
		  perf_type_str = "P5";
		  read_pmc_fns  = p5_read_pmc_fns;
		}
	      pcnt = &perf_cnt_p5;
	      break;

	    case 6:
	      perf_event_type = P6;
	      if (Cpu::model() >= 7)
		{
		  perfctr_type  = PERFCTR_X86_INTEL_PIII;
		  perf_type_str = "PIII";
		}
	      else if (Cpu::model() >= 3)
		{
		  perfctr_type  = PERFCTR_X86_INTEL_PII;
		  perf_type_str = "PII";
		}
	      else
		{
		  perfctr_type  = PERFCTR_X86_INTEL_P6;
		  perf_type_str = "PPro";
		}
	      read_pmc_fns = p6_read_pmc_fns;
	      pcnt = &perf_cnt_p6;
	      have_pce = 1;
	      break;

	    case 15:
	      perf_event_type = P4;
	      if (Cpu::model() >= 2)
		perfctr_type = PERFCTR_X86_INTEL_P4M2;
	      else
		perfctr_type = PERFCTR_X86_INTEL_P4;
	      perf_type_str = "P4";
	      read_pmc_fns = p4_read_pmc_fns;
	      pcnt = &perf_cnt_p4;
	      have_pce = 1;
	      break;
	    }
	}
      else if (Cpu::vendor() == Cpu::VENDOR_AMD)
	{
	  // AMD
	  switch (Cpu::family())
	    {
	    case 6:
	    case 15:
	      if (Cpu::family() == 15)
		{
		  perf_type_str = "K8";
		  perfctr_type  = PERFCTR_X86_AMD_K8;
		}
	      else
		{
		  perf_type_str = "K7";
		  perfctr_type  = PERFCTR_X86_AMD_K7;
		}
	      perf_event_type = P6;
	      read_pmc_fns    = k7_read_pmc_fns;
	      pcnt            = &perf_cnt_k7;
	      have_pce        = 1;
	      break;
	    }
	}

      // set PCE-Flag in CR4 to enable read of performace measurement
      // counters in usermode. PMC were introduced in Pentium MMX and
      // PPro processors.
      if (have_pce)
	Cpu::enable_rdpmc();
    }

  if (pcnt && (!pcnt->init()))
    {
      perfctr_type = PERFCTR_X86_GENERIC;
      pcnt         = 0;  // init failed, no performance counters available
    }

  // tell perflib the cpu type
  if (perfctr_set_cputype != 0)
    perfctr_set_cputype(perfctr_type);
}

PUBLIC static inline NOEXPORT void
Perf_cnt::set_pmc_fn(Mword slot, Mword nr)
{
  read_pmc_fn[slot] = read_pmc_fns[nr];
}

IMPLEMENT static inline int
Perf_cnt::have_watchdog()
{
  return (pcnt && pcnt->have_watchdog());
}

IMPLEMENT static inline void
Perf_cnt::setup_watchdog(Mword timeout)
{
  if (pcnt)
    pcnt->setup_watchdog(timeout);
}

IMPLEMENT static inline void
Perf_cnt::start_watchdog()
{
  if (pcnt && pcnt->watchdog_allocated())
    {
      pcnt->touch_watchdog();
      pcnt->start_watchdog();
    }
}

IMPLEMENT static inline void
Perf_cnt::stop_watchdog()
{
  if (pcnt && pcnt->watchdog_allocated())
    pcnt->stop_watchdog();
}

IMPLEMENT static inline void
Perf_cnt::touch_watchdog()
{
  if (pcnt && pcnt->watchdog_allocated())
    pcnt->touch_watchdog();
}

IMPLEMENT inline char const *
Perf_cnt::perf_type()
{
  return perf_type_str;
}

IMPLEMENT void
Perf_cnt::setup_pmc(Mword slot, Mword event, Mword user, Mword kern, Mword edge)
{
  if (pcnt)
    {
      Mword nr, bitmask, evntsel, unit_mask;
      const struct perfctr_event *pe = 0;

      split_event(event, &evntsel, &unit_mask);
      if (perfctr_lookup_event != 0)
	pe = perfctr_lookup_event(evntsel, &nr);
      bitmask = pe ? pe->counters_set : 0xffff;
      pcnt->setup_pmc(slot, bitmask, event, user, kern, edge);
      read_pmc[slot] = (kern | user) ? read_pmc_fn[slot] : dummy_read_pmc;
    }
}

// return type of performance registers we have
IMPLEMENT int
Perf_cnt::perf_mode(Mword slot, const char **mode, const char **name, 
		    Mword *event, Mword *user, Mword *kern, Mword *edge)
{
  if (perf_type() && pcnt)
    {
      Mword nr, evntsel, unit_mask;
      const struct perfctr_event *pe = 0;

      pcnt->perf_mode(slot, mode, event, user, kern, edge);
      split_event(*event, &evntsel, &unit_mask);
      if (perfctr_lookup_event != 0)
	pe = perfctr_lookup_event(evntsel, &nr);
      *name  = pe ? pe->name : "???";
      return 1;
    }

  *mode  = "";
  *event = *user = *kern  = 0;
  return 0;
}

PUBLIC static Mword
Perf_cnt::get_max_perf_event()
{
  return (perfctr_get_max_event != 0) ? perfctr_get_max_event() : 0;
}

PUBLIC static void
Perf_cnt::get_perf_event(Mword nr, Mword *evntsel, 
			 const char **name, const char **desc)
{
  const struct perfctr_event *pe = 0;

  if (perfctr_index_event != 0)
    pe = perfctr_index_event(nr);

  *name    = pe ? pe->name        : 0;
  *desc    = pe ? pe->description : 0;
  *evntsel = pe ? pe->evntsel     : 0;
}

PUBLIC static Mword
Perf_cnt::lookup_event(Mword evntsel)
{
  Mword nr;

  if (   (perfctr_lookup_event != 0)
      && (perfctr_lookup_event(evntsel, &nr) != 0))
    return nr;
  return (Mword)-1;
}

PUBLIC static void
Perf_cnt::get_unit_mask(Mword nr, Unit_mask_type *type,
			Mword *default_value, Mword *nvalues)
{
  const struct perfctr_event *event = 0;
  
  if (perfctr_index_event != 0) 
    event = perfctr_index_event(nr);

  *type = None;
  if (event && event->unit_mask)
    {
      *default_value = event->unit_mask->default_value;
      switch (event->unit_mask->type)
	{
	case perfctr_um_type_fixed:	*type = Fixed; break;
	case perfctr_um_type_exclusive:	*type = Exclusive; break;
	case perfctr_um_type_bitmask:	*type = Bitmask; break;
	}
      *nvalues = event->unit_mask->nvalues;
    }
}

PUBLIC static void
Perf_cnt::get_unit_mask_entry(Mword nr, Mword idx, 
			      Mword *value, const char **desc)
{
  const struct perfctr_event *event = 0;
  
  if (perfctr_index_event != 0)
    event = perfctr_index_event(nr);

  *value = 0;
  *desc  = 0;
  if (event && event->unit_mask && (idx < event->unit_mask->nvalues))
    {
      *value = event->unit_mask->values[idx].value;
      *desc  = event->unit_mask->values[idx].description;
    }
}

IMPLEMENT static void
Perf_cnt::split_event(Mword event, Mword *evntsel, Mword *unit_mask)
{
  switch (perf_event_type)
    {
    case P5:
      *evntsel   = event;
      *unit_mask = 0;
      break;
    case P6:
      *evntsel   =  event & 0x000000ff;
      *unit_mask = (event & 0x0000ff00) >> 8;
      break;
    case P4:
      break;
    }
}

IMPLEMENT static void
Perf_cnt::combine_event(Mword evntsel, Mword unit_mask, Mword *event)
{
  switch (perf_event_type)
    {
    case P5:
      *event = evntsel;
      break;
    case P6:
      *event = (evntsel & 0x000000ff) + ((unit_mask & 0x000000ff) << 8);
      break;
    case P4:
      break;
    }
}

