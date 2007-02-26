// Abstraction of Local APIC

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

  static bool ignore_invalid_reg_access;
  
private:
  Apic();			// default constructors are undefined
  Apic(const Apic&);
  
  static bool present;
  static const Address io_base;
  static unsigned timer_divisor;
  static Apic_type type;
  static unsigned frequency_khz;
  static unsigned max_lvt;
};

extern unsigned apic_spurious_interrupt_bug_cnt;
extern unsigned apic_spurious_interrupt_cnt;
extern unsigned apic_error_cnt;

#define APIC_ID                     0x20
#define   GET_APIC_ID(x)              (((x)>>24)&0x0F)
#define APIC_LVR                    0x30
#define   GET_APIC_VERSION(x)         ((x)&0xFF)
#define   APIC_INTEGRATED(x)          ((x)&0xF0)
#define   GET_APIC_MAXLVT(x)          (((x)>>16)&0xFF)
#define APIC_TASKPRI                0x80
#define   APIC_TPRI_MASK              0xFF
#define APIC_EOI                    0xB0
#define APIC_LDR                    0xD0
#define   APIC_LDR_MASK               (0xFF<<24)
#define APIC_DFR                    0xE0
#define   SET_APIC_DFR(x)             ((x)<<28)
#define APIC_SPIV                   0xF0
#define APIC_ESR                    0x280
#define APIC_LVTT                   0x320
#define APIC_LVTPC                  0x340
#define APIC_LVT0                   0x350
#define   SET_APIC_TIMER_BASE(x)      (((x)<<18))
#define   APIC_TIMER_BASE_DIV         0x2
#define APIC_LVT1                   0x360
#define APIC_LVTERR                 0x370
#define APIC_TMICT                  0x380
#define APIC_TMCCT                  0x390
#define APIC_TDCR                   0x3E0

#define APIC_LVT_MASKED             (1<<16)
#define APIC_LVT_TIMER_PERIODIC     (1<<17)
#define APIC_TDR_DIV_1              0xB
#define APIC_TDR_DIV_2              0x0
#define APIC_TDR_DIV_4              0x1
#define APIC_TDR_DIV_8              0x2
#define APIC_TDR_DIV_16             0x3
#define APIC_TDR_DIV_32             0x8
#define APIC_TDR_DIV_64             0x9
#define APIC_TDR_DIV_128            0xA

IMPLEMENTATION[ia32]:

#include <cstdio>
#include <cstdlib>
#include <cstring>

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

#define APIC_BASE_MSR               0x1b

unsigned apic_spurious_interrupt_bug_cnt;
unsigned apic_spurious_interrupt_cnt;
unsigned apic_error_cnt;
unsigned apic_io_base;

bool     Apic::ignore_invalid_reg_access;
bool     Apic::present;
const Address Apic::io_base = Kmem::local_apic_page;
unsigned Apic::timer_divisor = 1;
Apic::Apic_type Apic::type = APIC_NONE;
unsigned Apic::frequency_khz;
unsigned Apic::max_lvt;

PUBLIC static inline
unsigned
Apic::get_frequency_khz()
{
  return frequency_khz;
}

PUBLIC static inline
unsigned
Apic::reg_read(unsigned reg)
{
  return *((volatile unsigned*)(io_base + reg));
}

PUBLIC static inline
void
Apic::reg_write(unsigned reg, unsigned val)
{
  *((volatile unsigned*)(io_base + reg)) = val;
}

static inline
unsigned
Apic::timer_reg_read()
{
  return reg_read(APIC_TMCCT);
}

static inline
void
Apic::timer_reg_write(unsigned val)
{
  reg_read(APIC_TMICT);
  reg_write(APIC_TMICT, val/timer_divisor);
}

// check CPU type if APIC could be present
static inline
int
Apic::test_cpu()
{
  if (   !(Cpu::features() & FEAT_MSR)
      || !(Cpu::features() & FEAT_TSC))
    return 0;

  if (Cpu::vendor() == Cpu::VENDOR_INTEL)
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
  if (Cpu::vendor() == Cpu::VENDOR_AMD && Cpu::family() >= 6)
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
  unsigned tmp_val;
  
  tmp_val = reg_read(APIC_LVTT);
  tmp_val &= ~(APIC_LVT_MASKED);
  reg_write(APIC_LVTT, tmp_val);
}

PUBLIC static inline
void
Apic::timer_disable_irq()
{
  unsigned tmp_val;
  
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
  unsigned tmp_value;
    
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
  unsigned ver_reg = reg_read(APIC_LVR);

  return APIC_INTEGRATED(ver_reg)
		? GET_APIC_MAXLVT(ver_reg)
		: 2;
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
Apic::soft_enable()
{
  unsigned tmp_val;
  
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
Apic::err_enable()
{
  if (APIC_INTEGRATED(reg_read(APIC_LVR)))
    {
      unsigned tmp_val, before, after;

      if (max_lvt > 3)
	reg_write(APIC_ESR, 0);
      before = reg_read(APIC_ESR);

      tmp_val = reg_read(APIC_LVTERR);
      tmp_val &= 0xfffeff00;      // unmask error IRQ vector
      tmp_val |= 0x0000003e;      // Set error IRQ vector to 0x3e
      reg_write(APIC_LVTERR, tmp_val);

      if (max_lvt > 3)
	reg_write(APIC_ESR, 0);
      after = reg_read(APIC_ESR);
      printf("APIC ESR value before/after enabling: %08x/%08x\n", 
	    before, after);
    }
}

// activate APIC after activating by MSR was successful
// see "Intel Architecture Software Developer's Manual,
//      Volume 3: System Programming Guide, Appendix E"
static FIASCO_INIT
void
Apic::activate_by_io()
{
  unsigned tmp_val;

  Proc::Status flags = Proc::cli_save();

  // mask 8259 interrupts
  Pic::Status old_irqs = Pic::disable_all_save();

  soft_enable();

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

  Pic::restore_all( old_irqs );

  // unmask 8259 interrupts
  Proc::sti_restore(flags);  
}

// activate APIC by writing to appropriate MSR
static FIASCO_INIT
void
Apic::activate_by_msr()
{
  Unsigned64 msr;

  msr = Cpu::rdmsr(APIC_BASE_MSR);
  msr |= 0x800;                         // activate APIC
  msr &= 0x00000fff;
  msr |= (io_base & 0xfffff000); // set address
  Cpu::wrmsr(msr, APIC_BASE_MSR);

  // now the CPU feature flags may have changed
  Cpu::identify();
}

#if 0
// deactivate APIC by writing to appropriate MSR
static
void
Apic_deactivate_by_msr()
{
  Unsigned64 msr;
 
  msr = Cpu::rdmsr(APIC_BASE_MSR);
  low  &= 0xfffff7ff; // deactivate APIC
  Cpu::wrmsr(msr, APIC_BASE_MSR);
}
#endif

// check if we still receive interrupts after we changed the IRQ routing
static FIASCO_INIT
int
Apic::check_still_getting_irqs()
{
  Unsigned64 tsc_until;
  Cpu_time clock_start = Kmem::info()->clock;
  
  tsc_until = Cpu::rdtsc();
  tsc_until += 0x01000000; // > 10 Mio cycles should be sufficient until
                           // we have processors with more than 10 GHz
  do
    {
      // kernel clock by timer interrupt updated?
      if (Kmem::info()->clock != clock_start)
	// yes, succesful
	return 1;
    } while (Cpu::rdtsc() < tsc_until);
  
  // timeout
  return 0;
}

// Local APIC is disabled on newer Mainboards, try to activate
static FIASCO_INIT
int
Apic::try_to_activate()
{
  activate_by_msr();
  if (   test_present()
      && check_working())
    {
      printf("APIC disabled, re-enabling... ");
      activate_by_io();
      if (check_still_getting_irqs())
	{
	  printf("sucessful!\n");
	  return 1;
	}
      
      panic("unsuccessful!\n");
    }

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
int
Apic::have_pcint()
{
  return (present && (max_lvt >= 4));
}

PUBLIC static
void
Apic::set_perf_nmi()
{
  if (have_pcint())
    reg_write(APIC_LVTPC, 0x400);
  else
    panic("Trying to set LVTPC");
}

static FIASCO_INIT
void
Apic::calibrate_timer()
 {
  frequency_khz = 0;

  timer_disable_irq();
  timer_set_divisor(1);
  timer_reg_write(1000000000);
  
  Pit::setup_channel2_to_200hz();

  Unsigned32 count, tt1, tt2, result;

  tt1 = timer_reg_read();
  count = 0;
  do 
    {
      count++;
    } while ((Io::in8(0x61) & 0x20) == 0);
  tt2 = timer_reg_read();

  result = (tt1-tt2) * timer_divisor;

  // Error: ECTCNEVERSET
  if (count <= 1)
    return;

  // Error: ECPUTOOSLOW
  if (result <= 50)
    return;

  asm volatile ("divl %1"
		: "=a" (result)  
		:  "r" (50), "0" (result), "d" (0));

  frequency_khz = result;
}

extern "C" void
apic_error_interrupt(Return_frame *ret_regs)
{
  unsigned err1, err2;

  // we are entering with disabled interrupts
  err1 = Apic::reg_read(APIC_ESR);
  Apic::reg_write(APIC_ESR, 0);
  err2 = Apic::reg_read(APIC_ESR);
  Apic::irq_ack();

  cpu_lock.clear();

  if (err1 == 0x80 || err2 == 0x80)
    {
      // ignore possible invalid access which may happen in
      // jdb::do_dump_memory()
      if (Apic::ignore_invalid_reg_access)
	return;

      printf("APIC invalid register access error at %08x\n", ret_regs->eip);
      return;
    }

  apic_error_cnt++;
  printf("APIC error %08x(%08x)\n", err1, err2);
}

IMPLEMENT
void Apic::init()
{
  if (present = test_cpu())
    {
      if ((present = test_present() && check_working()))
	{
	  activate_by_msr();
	  soft_enable();
	}
      else
	present = try_to_activate();
    }

  if (present)
    {
      max_lvt = get_maxlvt();     // determine number of local interrupts
      apic_io_base = Kmem::local_apic_page;

      calibrate_timer();

      printf("Found Local APIC version 0x%02x id 0x%02x\n",
	  GET_APIC_VERSION(reg_read(APIC_LVR)),
	  GET_APIC_ID(reg_read(APIC_ID)));

      err_enable();
    }
  else
    {
      panic("Local APIC not found");
    }
}
