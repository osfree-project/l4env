INTERFACE:

#include "types.h"
#include "initcalls.h"

class Return_frame;

class Apic
{
public:
  static void init() FIASCO_INIT;

  typedef enum
    {
      APIC_NONE,
      APIC_P6,			// Intel PPro, PIII
      APIC_P4,			// Intel PIV
      APIC_K7			// AMD Athlon Model 2
    } Apic_type;

private:
  Apic();			// default constructors are undefined
  Apic(const Apic&);
  static void			error_interrupt(Return_frame *regs)
				asm ("apic_error_interrupt") FIASCO_FASTCALL;

  static int			present;
  static int			good_cpu;
  static const			Address io_base;
  static Address		phys_base;
  static unsigned		timer_divisor;
  static Apic_type		type;
  static unsigned		frequency_khz;
  static unsigned		max_lvt;
  static Unsigned32		scaler_ns_to_apic;

  enum
    {
      APIC_ID			= 0x20,
      APIC_LVR			= 0x30,
      APIC_TASKPRI		= 0x80,
      APIC_TPRI_MASK		= 0xFF,
      APIC_EOI			= 0xB0,
      APIC_LDR			= 0xD0,
      APIC_LDR_MASK		= (0xFF<<24),
      APIC_DFR			= 0xE0,
      APIC_SPIV			= 0xF0,
      APIC_ISR			= 0x100,
      APIC_TMR			= 0x180,
      APIC_IRR			= 0x200,
      APIC_ESR			= 0x280,
      APIC_LVTT			= 0x320,
      APIC_LVTTHMR		= 0x330,
      APIC_LVTPC		= 0x340,
      APIC_LVT0			= 0x350,
      APIC_TIMER_BASE_DIV	= 0x2,
      APIC_LVT1			= 0x360,
      APIC_LVTERR		= 0x370,
      APIC_TMICT		= 0x380,
      APIC_TMCCT		= 0x390,
      APIC_TDCR			= 0x3E0,

      APIC_SND_PENDING		= (1<<12),
      APIC_INPUT_POLARITY	= (1<<13),
      APIC_LVT_REMOTE_IRR	= (1<<14),
      APIC_LVT_LEVEL_TRIGGER	= (1<<15),
      APIC_LVT_MASKED		= (1<<16),
      APIC_LVT_TIMER_PERIODIC	= (1<<17),
      APIC_TDR_DIV_1		= 0xB,
      APIC_TDR_DIV_2		= 0x0,
      APIC_TDR_DIV_4		= 0x1,
      APIC_TDR_DIV_8		= 0x2,
      APIC_TDR_DIV_16		= 0x3,
      APIC_TDR_DIV_32		= 0x8,
      APIC_TDR_DIV_64		= 0x9,
      APIC_TDR_DIV_128		= 0xA,
    };

  enum
    {
      MASK			=  1,
      TRIGGER_MODE		=  2,
      REMOTE_IRR		=  4,
      PIN_POLARITY		=  8,
      DELIVERY_STATE		= 16,
      DELIVERY_MODE		= 32,
    };

  enum
    {
      APIC_BASE_MSR		= 0x1b,
    };
};

extern unsigned apic_spurious_interrupt_bug_cnt;
extern unsigned apic_spurious_interrupt_cnt;
extern unsigned apic_error_cnt;

IMPLEMENTATION[ia32]:

#include <assert.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "config.h"
#include "cpu.h"
#include "cpu_lock.h"
#include "entry_frame.h"
#include "globals.h"
#include "io.h"
#include "kmem.h"
#include "panic.h"
#include "processor.h"
#include "regdefs.h"
#include "pic.h"
#include "pit.h"

unsigned apic_spurious_interrupt_bug_cnt;
unsigned apic_spurious_interrupt_cnt;
unsigned apic_error_cnt;
unsigned apic_io_base;

int        Apic::present;
int        Apic::good_cpu;
const Address Apic::io_base = Mem_layout::Local_apic_page;
Address    Apic::phys_base;
unsigned   Apic::timer_divisor = 1;
Apic::Apic_type Apic::type = APIC_NONE;
unsigned   Apic::frequency_khz;
unsigned   Apic::max_lvt;
Unsigned32 Apic::scaler_ns_to_apic;

int ignore_invalid_apic_reg_access;

PRIVATE static inline
Unsigned32
Apic::get_id()
{
  return (reg_read(APIC_ID) >> 24) & 0x0f;
}

PRIVATE static inline
Unsigned32
Apic::get_version()
{
  return reg_read(APIC_LVR) & 0xFF;
}

PRIVATE static inline NOEXPORT
int
Apic::is_integrated()
{
  return reg_read(APIC_LVR) & 0xF0;
}

PRIVATE static inline NOEXPORT
Unsigned32
Apic::get_max_lvt()
{
  return ((reg_read(APIC_LVR) >> 16) & 0xFF);
}

PRIVATE static inline NOEXPORT
Unsigned32
Apic::get_num_errors()
{
  return reg_read(APIC_ESR);
}

PRIVATE static inline NOEXPORT
void
Apic::clear_num_errors()
{
  reg_write(APIC_ESR, 0);
}

PUBLIC static inline
unsigned
Apic::get_frequency_khz()
{
  return frequency_khz;
}

PUBLIC static inline
Unsigned32
Apic::reg_read(unsigned reg)
{
  return *((volatile unsigned*)(io_base + reg));
}

PUBLIC static inline
void
Apic::reg_write(unsigned reg, Unsigned32 val)
{
  *((volatile Unsigned32*)(io_base + reg)) = val;
}

PUBLIC static inline
int
Apic::reg_delivery_mode(Unsigned32 val)
{
  return (val >> 8) & 7;
}

PUBLIC static inline
int
Apic::reg_lvt_vector(Unsigned32 val)
{
  return val & 0xff;
}

PUBLIC static inline
Unsigned32
Apic::timer_reg_read()
{
  return reg_read(APIC_TMCCT);
}

PUBLIC static inline
Unsigned32
Apic::timer_reg_read_initial()
{
  return reg_read(APIC_TMICT);
}

PUBLIC static inline
void
Apic::timer_reg_write(Unsigned32 val)
{
  reg_read(APIC_TMICT);
  reg_write(APIC_TMICT, val);
}

PUBLIC static inline
Unsigned32
Apic::ns_to_apic(Unsigned64 ns)
{
  Unsigned32 apic, dummy1, dummy2;
  asm ("movl  %%edx, %%ecx		\n\t"
       "mull  %4			\n\t"
       "movl  %%ecx, %%eax		\n\t"
       "movl  %%edx, %%ecx		\n\t"
       "mull  %4			\n\t"
       "addl  %%ecx, %%eax		\n\t"
      :"=a" (apic), "=d" (dummy1), "=&c" (dummy2)
      : "A" (ns),   "g" (scaler_ns_to_apic)
       );
  return apic;
}

// set the global pagetable entry for the Local APIC device registers
static FIASCO_INIT
void
Apic::map_apic_page()
{
  Address offs;

  // We should not change the physical address of the Local APIC
  // page if possible since VMware would complain about a non-
  // implemented feature
  Kmem::map_devpage_4k(phys_base, Mem_layout::Local_apic_page,
		       false, true, &offs);

  assert(offs == 0);
}

// check CPU type if APIC could be present
static FIASCO_INIT
int
Apic::test_cpu()
{
  if (   !(Cpu::features() & FEAT_MSR)
      || !(Cpu::features() & FEAT_TSC))
    return 0;

  if (Cpu::vendor() == Cpu::Vendor_intel)
    {
      if (Cpu::family() == 15)
	{
	  // Intel PIV
	  type = APIC_P4;
	  return 1;
	}
      else if (Cpu::family() >= 6)
	{
	  // Intel PPro, PIII
	  type = APIC_P6;
	  return 1;
	}
    }
  if (Cpu::vendor() == Cpu::Vendor_amd && Cpu::family() >= 6)
    {
      // >= AMD K7 Model 2
      type = APIC_K7;
      return 1;
    }

  return 0;
}

// test if APIC present
static inline
int
Apic::test_present()
{
  return Cpu::features() & FEAT_APIC;
}

PUBLIC static inline
void
Apic::timer_enable_irq()
{
  Unsigned32 tmp_val;
  
  tmp_val = reg_read(APIC_LVTT);
  tmp_val &= ~(APIC_LVT_MASKED);
  reg_write(APIC_LVTT, tmp_val);
}

PUBLIC static inline
void
Apic::timer_disable_irq()
{
  Unsigned32 tmp_val;
  
  tmp_val = reg_read(APIC_LVTT);
  tmp_val |= APIC_LVT_MASKED;
  reg_write(APIC_LVTT, tmp_val);
}

PUBLIC static inline
int
Apic::timer_is_irq_enabled()
{
  return ~reg_read(APIC_LVTT) & APIC_LVT_MASKED;
}

PUBLIC static inline
void
Apic::timer_set_periodic()
{
  Unsigned32 tmp_val = reg_read(APIC_LVTT);
  tmp_val |= APIC_LVT_TIMER_PERIODIC;
  reg_write(APIC_LVTT, tmp_val);
}

PUBLIC static inline
void
Apic::timer_set_one_shot()
{
  Unsigned32 tmp_val = reg_read(APIC_LVTT);
  tmp_val &= ~APIC_LVT_TIMER_PERIODIC;
  reg_write(APIC_LVTT, tmp_val);
}

PUBLIC static inline
void
Apic::timer_assign_irq_vector(unsigned vector)
{
  Unsigned32 tmp_val = reg_read(APIC_LVTT);
  tmp_val &= 0xffffff00;
  tmp_val |= vector;
  reg_write(APIC_LVTT, tmp_val);
}

PUBLIC static inline
void
Apic::irq_ack()
{
  reg_read(APIC_SPIV);
  reg_write(APIC_EOI, 0);
}

static
void
Apic::timer_set_divisor(unsigned newdiv)
{
  int i;
  int div = -1;
  int divval = newdiv;
  Unsigned32 tmp_value;
    
  static int divisor_tab[8] = 
    {
      APIC_TDR_DIV_1,  APIC_TDR_DIV_2,  APIC_TDR_DIV_4,  APIC_TDR_DIV_8,
      APIC_TDR_DIV_16, APIC_TDR_DIV_32, APIC_TDR_DIV_64, APIC_TDR_DIV_128 
    };

  for (i=0; i<8; i++)
    {
      if (divval & 1)
	{
	  if (divval & ~1)
	    {
	      printf("bad APIC divisor %d\n", newdiv);
	      return;
	    }
	  div = divisor_tab[i];
	  break;
	}
      divval >>= 1;
    }
    
  if (div != -1)
    {
      timer_divisor = newdiv;
      tmp_value = reg_read(APIC_TDCR);
      tmp_value &= ~0x1F;
      tmp_value |= div;
      reg_write(APIC_TDCR, tmp_value);
    }
}

static FIASCO_INIT
int
Apic::get_maxlvt()
{
  return is_integrated() ? get_max_lvt() : 2;
}

PUBLIC static inline
int
Apic::have_pcint()
{
  return (present && (max_lvt >= 4));
}

PUBLIC static inline
int
Apic::have_tsint()
{
  return (present && (max_lvt >= 5));
}

// check if APIC is working (check timer functionality)
static FIASCO_INIT
int
Apic::check_working()
{
  Unsigned64 tsc_until;

  timer_disable_irq();
  timer_set_divisor(1);
  timer_reg_write(0x10000000);
  
  tsc_until = Cpu::rdtsc() + 0x100;  // we only have to wait for one bus cycle

  do 
    {
      if (timer_reg_read() != 0x10000000)
	return 1;
    } while (Cpu::rdtsc() < tsc_until);

  return 0;
}

static FIASCO_INIT
void
Apic::init_spiv()
{
  Unsigned32 tmp_val;
  
  tmp_val = reg_read(APIC_SPIV);
  tmp_val |= (1<<8);          // enable APIC
  tmp_val &= ~(1<<9);         // enable Focus Processor Checking
  tmp_val &= ~0xff;
  tmp_val |= 0x3f;            // Set spurious IRQ vector to 0x3f
                              // bit 0..3 are hardwired to 1 on PPro!
  reg_write(APIC_SPIV, tmp_val);
}

// activate APIC error interrupt
static FIASCO_INIT
void
Apic::enable_errors()
{
  if (is_integrated())
    {
      Unsigned32 tmp_val, before, after;

      if (max_lvt > 3)
	clear_num_errors();
      before = get_num_errors();

      tmp_val = reg_read(APIC_LVTERR);
      tmp_val &= 0xfffeff00;      // unmask error IRQ vector
      tmp_val |= 0x0000003e;      // Set error IRQ vector to 0x3e
      reg_write(APIC_LVTERR, tmp_val);

      if (max_lvt > 3)
	clear_num_errors();
      after = get_num_errors();
      printf("APIC ESR value before/after enabling: %08x/%08x\n", 
	    before, after);
    }
}

// activate APIC after activating by MSR was successful
// see "Intel Architecture Software Developer's Manual,
//      Volume 3: System Programming Guide, Appendix E"
static FIASCO_INIT
void
Apic::route_pic_through_apic()
{
  Unsigned32 tmp_val;

  // mask 8259 interrupts
  Proc::Status flags = Proc::cli_save();
  Pic::Status old_irqs = Pic::disable_all_save();

  // set LINT0 to ExtINT, edge triggered
  tmp_val = reg_read(APIC_LVT0);
  tmp_val &= 0xfffe5800;
  tmp_val |= 0x00000700;
  reg_write(APIC_LVT0, tmp_val);
    
  // set LINT1 to NMI, edge triggered
  tmp_val = reg_read(APIC_LVT1);
  tmp_val &= 0xfffe5800;
  tmp_val |= 0x00000400;
  reg_write(APIC_LVT1, tmp_val);

  // unmask 8259 interrupts
  Pic::restore_all(old_irqs);
  Proc::sti_restore(flags);

  printf("APIC was disabled --- routing PIC through APIC\n");
}

static FIASCO_INIT
void
Apic::init_lvt()
{
  Unsigned32 tmp_val;

  Proc::Status flags = Proc::cli_save();
  // mask timer interrupt and set vector to _not_ invalid value
  tmp_val  = reg_read(APIC_LVTT);
  tmp_val |= APIC_LVT_MASKED;
  tmp_val |= 0xff;
  reg_write(APIC_LVTT, tmp_val);
  if (have_pcint())
    {
      // mask performance interrupt and set vector to a valid value
      tmp_val  = reg_read(APIC_LVTPC);
      tmp_val |= APIC_LVT_MASKED;
      tmp_val |= 0xff;
      reg_write(APIC_LVTPC, tmp_val);
    }
  if (have_tsint())
    {
      // mask thermal sensor interrupt and set vector to a valid value
      tmp_val  = reg_read(APIC_LVTTHMR);
      tmp_val |= APIC_LVT_MASKED;
      tmp_val |= 0xff;
      reg_write(APIC_LVTTHMR, tmp_val);
    }
  Proc::sti_restore(flags);  
}

// give us a hint if we have an APIC but it is disabled
PUBLIC static
int
Apic::test_present_but_disabled()
{
  if (!good_cpu)
    return 0;

  Unsigned64 msr = Cpu::rdmsr(APIC_BASE_MSR);
  return ((msr & 0xffffff000ULL) == 0xfee00000ULL);
}

// activate APIC by writing to appropriate MSR
static FIASCO_INIT
void
Apic::activate_by_msr()
{
  Unsigned64 msr;

  msr = Cpu::rdmsr(APIC_BASE_MSR);
  phys_base = msr & 0xfffff000;
  msr |= (1<<11);
  Cpu::wrmsr(msr, APIC_BASE_MSR);

  // now the CPU feature flags may have changed
  Cpu::identify();
}

// check if we still receive interrupts after we changed the IRQ routing
PUBLIC static FIASCO_INIT
int
Apic::check_still_getting_interrupts()
{
  if (!Config::apic)
    return 0;

  Unsigned64 tsc_until;
  Cpu_time clock_start = Kip::k()->clock;

  tsc_until = Cpu::rdtsc();
  tsc_until += 0x01000000; // > 10 Mio cycles should be sufficient until
                           // we have processors with more than 10 GHz
  do
    {
      // kernel clock by timer interrupt updated?
      if (Kip::k()->clock != clock_start)
	// yes, succesful
	return 1;
    } while (Cpu::rdtsc() < tsc_until);
  
  // timeout
  return 0;
}

PUBLIC static
inline int
Apic::is_present()
{
  return present;
}

PUBLIC static
inline int
Apic::cpu_type()
{
  return type;
}

PUBLIC static
void
Apic::set_perf_nmi()
{
  if (have_pcint())
    reg_write(APIC_LVTPC, 0x400);
}

static FIASCO_INIT
void
Apic::calibrate_timer()
{
  const unsigned calibrate_time = 50;

  frequency_khz = 0;

  timer_disable_irq();
  timer_set_divisor(1);
  timer_reg_write(1000000000);

  Proc::Status o = Proc::cli_save();
  Pit::setup_channel2_to_20hz();

  Unsigned32 count, tt1, tt2, result, dummy;

  count = 0;

  tt1 = timer_reg_read();
  do 
    {
      count++;
    } while ((Io::in8(0x61) & 0x20) == 0);
  tt2 = timer_reg_read();

  Proc::sti_restore(o);

  result = (tt1-tt2) * timer_divisor;

  // APIC not running
  if (count <= 1)
    return;

  asm ("divl %2"
      :"=a" (frequency_khz), "=d" (dummy)
      : "r" (calibrate_time), "a" (result), "d" (0));

  if (frequency_khz >= 1000000)
    panic("APIC frequency too high, adapt Apic::scaler_ns_to_apic");

  Kip::k()->frequency_bus = frequency_khz;
  scaler_ns_to_apic           = Cpu::muldiv(1<<31, frequency_khz, 1000000>>1);
}

IMPLEMENT
void
Apic::error_interrupt(Return_frame *regs)
{
  Unsigned32 err1, err2;

  // we are entering with disabled interrupts
  err1 = Apic::get_num_errors();
  Apic::clear_num_errors();
  err2 = Apic::get_num_errors();
  Apic::irq_ack();

  cpu_lock.clear();

  if (err1 == 0x80 || err2 == 0x80)
    {
      // ignore possible invalid access which may happen in
      // jdb::do_dump_memory()
      if (ignore_invalid_apic_reg_access)
	return;

      printf("APIC invalid register access error at "L4_PTR_FMT"\n",
	     regs->ip());
      return;
    }

  apic_error_cnt++;
  printf("APIC error %08x(%08x)\n", err1, err2);
}

// deactivate APIC by writing to appropriate MSR
PUBLIC static
void
Apic::done()
{
  Unsigned64 val;

  if (!present)
    return;

  val = reg_read(APIC_SPIV);
  val &= ~(1<<8);
  reg_write(APIC_SPIV, val);
 
  val = Cpu::rdmsr(APIC_BASE_MSR);
  val  &= ~(1<<11);
  Cpu::wrmsr(val, APIC_BASE_MSR);
}

IMPLEMENT
void
Apic::init()
{
  int was_present;

  good_cpu = test_cpu();

  if (!Config::apic)
    return;

  // check CPU type
  if ((present = good_cpu))
    {
      // check cpu features of cpuid
      was_present = test_present();

      // activate; this could lead an disabled APIC to appear
      // set base address of I/O registers to be able to access the registers
      activate_by_msr();

      // previous test_present() could have failed but we could have it
      // activated by writing the msr so we have to test again
      if ((present = test_present()))
	{
	  // map the Local APIC device registers
	  map_apic_page();

	  // determine number of local interrupts
	  max_lvt = get_maxlvt();

	  // set some interrupt vectors to appropriate values
	  init_lvt();

	  // initialize APIC_SPIV register
	  init_spiv();

	  // test if local timer counts down
	  if ((present = check_working()))
	    {
	      if (!was_present)
		// APIC _was_ not present before writing to msr so we have
		// to set APIC_LVT0 and APIC_LVT1 to appropriate values
		route_pic_through_apic();
	    }
	}
    }

  if (!present)
    panic("Local APIC not found");

  apic_io_base = Mem_layout::Local_apic_page;
  calibrate_timer();
  printf("Found Local APIC version 0x%02x id 0x%02x\n", 
      get_version(), get_id());
  timer_set_divisor(1);
  enable_errors();
}
