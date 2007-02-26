/*
 * Fiasco ia32-native
 * Architecture specific cpu init code
 */

INTERFACE:

#include "l4_types.h"
#include "initcalls.h"

class Gdt;
class Tss;

EXTENSION class Cpu 
{
public:
  enum Lbr
    {
      LBR_UNINITIALIZED = 0,
      LBR_NONE,
      LBR_P6,
      LBR_P4,
    };

private:
  static Lbr		lbr			asm ("CPU_LBR");
  typedef void		(Sysenter)(void);
  static Sysenter	*current_sysenter	asm ("CURRENT_SYSENTER");
  static Gdt		*gdt			asm ("CPU_GDT");
  static Tss		*tss			asm ("CPU_TSS");
  static Tss		*tss_dbf;
};

IMPLEMENTATION[ia32]:

#include "boot_info.h"
#include "config.h"
#include "gdt.h"
#include "globals.h"
#include "initcalls.h"
#include "io.h"
#include "pit.h"
#include "processor.h"
#include "regdefs.h"
#include "tss.h"

Cpu::Lbr       Cpu::lbr;
Cpu::Sysenter *Cpu::current_sysenter;
Gdt           *Cpu::gdt;
Tss           *Cpu::tss;
Tss           *Cpu::tss_dbf;

extern "C" void entry_sysenter (void);
extern "C" void entry_sysenter_c (void);


PUBLIC static inline
void
Cpu::set_cr0 (Unsigned32 val)
{ asm volatile ("movl %0, %%cr0" : : "r" (val)); }

PUBLIC static inline
void
Cpu::set_pdbr (Address addr)
{ asm volatile ("movl %0, %%cr3" : : "r" (addr)); }

PUBLIC static inline
void
Cpu::set_cr4 (Unsigned32 val)
{ asm volatile ("movl %0, %%cr4" : : "r" (val)); }

PUBLIC static inline
void
Cpu::set_ldt (Unsigned16 val)
{ asm volatile ("lldt %0" : : "rm" (val)); }

PUBLIC static inline
void
Cpu:: set_cs ()
{
  asm volatile ("ljmp %0,$1f ; 1:" 
		: : "i"(Gdt::gdt_code_kernel | Gdt::Selector_kernel));
}

PUBLIC static inline
void
Cpu::set_ss (Unsigned32 val)
{ asm volatile ("movl %0, %%ss" : : "rm" (val)); }

PUBLIC static inline
void
Cpu::set_tr (Unsigned16 val)
{ asm volatile ("ltr %0" : : "rm" (val)); }

PUBLIC static inline
Mword
Cpu::get_cr0 ()
{ Mword val; asm volatile ("movl %%cr0, %0" : "=r" (val)); return val; }

PUBLIC static inline
Address
Cpu::get_pdbr ()
{ Address addr; asm volatile ("movl %%cr3, %0" : "=r" (addr)); return addr; }

PUBLIC static inline
Mword
Cpu::get_cr4 ()
{ Mword val; asm volatile ("movl %%cr4, %0" : "=r" (val)); return val; }

PUBLIC static inline
Unsigned16
Cpu::get_ldt ()
{ Unsigned16 val; asm volatile ("sldt %0" : "=rm" (val)); return val; }

PUBLIC static inline
Unsigned16
Cpu::get_tr ()
{ Unsigned16 val; asm volatile ("str %0" : "=rm" (val)); return val; }

IMPLEMENT inline
int
Cpu::can_wrmsr ()
{ return features() & FEAT_MSR; }

PUBLIC static inline
Unsigned64
Cpu::rdmsr (Unsigned32 reg)
{
  Unsigned64 msr;

  asm volatile ("rdmsr" : "=A" (msr) : "c" (reg));
  return msr;
}

PUBLIC static inline
Unsigned64
Cpu::rdpmc (Unsigned32 idx, Unsigned32)
{
  Unsigned64 pmc;

  asm volatile ("rdpmc" : "=A" (pmc) : "c" (idx));
  return pmc;
}

PUBLIC static inline
void
Cpu::wrmsr (Unsigned32 low, Unsigned32 high, Unsigned32 reg)
{ asm volatile ("wrmsr" : : "a" (low), "d" (high), "c" (reg)); }

PUBLIC static inline
void
Cpu::wrmsr (Unsigned64 msr, Unsigned32 reg)
{ asm volatile ("wrmsr" : : "A" (msr), "c" (reg)); }

PUBLIC static inline
void
Cpu::enable_rdpmc()
{ set_cr4(get_cr4() | CR4_PCE); }

PUBLIC static
Cpu::Lbr const
Cpu::lbr_type()
{
  if (lbr == LBR_UNINITIALIZED)
    {
      lbr = LBR_NONE;

      if (features() & FEAT_MSR)
	{
	  // Intel
	  if (vendor() == Vendor_intel)
	    {
	      if (family() == 15)
		lbr = LBR_P4; // P4
	      else if (family() >= 6)
		lbr = LBR_P6; // PPro, PIII
	    }
	  else if (vendor() == Vendor_amd)
	    {
	      if ((family() == 6) || (family() == 15))
		lbr = LBR_P6; // K7/K8
	    }
	}
    }

  return lbr;
}

PUBLIC static inline
void
Cpu::enable_lbr (void)
{
  if (lbr_type() != LBR_NONE)
    {
      Unsigned64 lbr_ctrl = rdmsr (0x1d9);
      wrmsr (lbr_ctrl | 1, 0x1d9);
    }
}

PUBLIC static inline
void
Cpu::disable_lbr (void)
{
  if (lbr_type() != LBR_NONE)
    {
      Unsigned64 lbr_ctrl = rdmsr (0x1d9);
      wrmsr (lbr_ctrl & ~1, 0x1d9);
    }
}

IMPLEMENT FIASCO_INIT
void
Cpu::init (void)
{
  identify();

  calibrate_tsc();

  if (scaler_tsc_to_ns)
    _frequency = ns_to_tsc(1000000000UL);

  unsigned cr4 = get_cr4();

  if (features() & FEAT_FXSR)
    cr4 |= CR4_OSFXSR;

  if (features() & FEAT_SSE)
    cr4 |= CR4_OSXMMEXCPT;

  set_cr4 (cr4);

  // reset time stamp counter (better for debugging)
  if (features() & FEAT_TSC)
    wrmsr(0, 0, 0x10);
}

PUBLIC static FIASCO_INIT
void
Cpu::init_sysenter (Address kernel_esp)
{
  // Check for Sysenter/Sysexit Feature
  if (have_sysenter())
    {
      wrmsr (Gdt::gdt_code_kernel, 0, SYSENTER_CS_MSR);
      wrmsr ((Unsigned32) kernel_esp, 0, SYSENTER_ESP_MSR);
      if (Config::Assembler_ipc_shortcut)
	set_sysenter(entry_sysenter);
      else
	set_sysenter(entry_sysenter_c);
    }
}

PUBLIC static
void
Cpu::set_sysenter (void (*func)(void))
{
  // Check for Sysenter/Sysexit Feature
  if (have_sysenter())
    {
      wrmsr ((Unsigned32) func, 0, SYSENTER_EIP_MSR);
      current_sysenter = func;
    }
}

PUBLIC static
void
Cpu::get_sysenter (void (**func)(void))
{
  *func = current_sysenter;
}

// Return 2^32 / (tsc clocks per usec)
static FIASCO_INIT
void
Cpu::calibrate_tsc ()
{
  const unsigned calibrate_time = 50000 /*us*/ + 1;

  // sanity check
  if (! (features() & FEAT_TSC))
    goto bad_ctc;

  Unsigned64 tsc_start, tsc_end;
  Unsigned32 count, tsc_to_ns_div, dummy;

    {
      // disable interrupts
      Proc::Status o = Proc::cli_save();

      Pit::setup_channel2_to_20hz();

      tsc_start = rdtsc ();
      count = 0;
      do
	{
	  count++;
	}
      while ((Io::in8 (0x61) & 0x20) == 0);
      tsc_end = rdtsc ();

      // restore flags
      Proc::sti_restore(o);
    }

  // Error: ECTCNEVERSET
  if (count <= 1)
    goto bad_ctc;

  tsc_end -= tsc_start;

  // prevent overflow in division (CPU too fast)
  if (tsc_end & 0xffffffff00000000LL)
    goto bad_ctc;

  // prevent overflow in division (CPU too slow)
  if ((tsc_end & 0xffffffffL) < calibrate_time)
    goto bad_ctc;

  // tsc_to_ns_div = calibrate_time * 2^32 / tsc
  asm ("divl %2"
       :"=a" (tsc_to_ns_div), "=d" (dummy)
       :"r" ((Unsigned32)tsc_end), "a" (0), "d" (calibrate_time));

  scaler_tsc_to_ns = muldiv (tsc_to_ns_div, 1000, 1<<5);
  scaler_tsc_to_us =         tsc_to_ns_div;
  scaler_ns_to_tsc = muldiv (1<<31, ((Unsigned32)tsc_end),
			     calibrate_time * 1000>>1 * 1<<5);

  return;

bad_ctc:
  if (Config::kinfo_timer_uses_rdtsc)
    panic("Can't calibrate tsc");
}

IMPLEMENT inline
Unsigned64
Cpu::time_us ()
{
  return tsc_to_us (rdtsc());
}

PUBLIC static FIASCO_INIT
void
Cpu::init_gdt (Address gdt_mem, Address user_max)
{
  gdt = reinterpret_cast<Gdt*>(gdt_mem);

  // make sure kernel cs/ds and user cs/ds are placed in the same
  // cache line, respectively; pre-set all "accessed" flags so that
  // the CPU doesn't need to do this later

  gdt->set_entry_4k (Gdt::gdt_code_kernel/8, 0, 0xffffffff,
		     Gdt_entry::Access_kernel | 
		     Gdt_entry::Access_code_read |
  		     Gdt_entry::Accessed, Gdt_entry::Size_32);
  gdt->set_entry_4k (Gdt::gdt_data_kernel/8, 0, 0xffffffff,
		     Gdt_entry::Access_kernel | 
	  	     Gdt_entry::Access_data_write | 
  		     Gdt_entry::Accessed, Gdt_entry::Size_32);
  gdt->set_entry_4k (Gdt::gdt_code_user/8, 0, user_max,
		     Gdt_entry::Access_user | 
		     Gdt_entry::Access_code_read | 
		     Gdt_entry::Accessed, Gdt_entry::Size_32);
  gdt->set_entry_4k (Gdt::gdt_data_user/8, 0, user_max,
		     Gdt_entry::Access_user |
	  	     Gdt_entry::Access_data_write | 
  		     Gdt_entry::Accessed, Gdt_entry::Size_32);
}

PUBLIC static FIASCO_INIT
void
Cpu::init_tss (Address tss_mem, size_t tss_size)
{
  tss = reinterpret_cast<Tss*>(tss_mem);

  gdt->set_entry_byte (Gdt::gdt_tss/8, tss_mem, tss_size,
		       Gdt_entry::Access_kernel | Gdt_entry::Access_tss, 0);

  tss->ss0 = Gdt::gdt_data_kernel;
  tss->io_bit_map_offset = Mem_layout::Io_bitmap - tss_mem;
}

extern "C" void entry_vec08_dbf ();
extern "C" Address dbf_stack_top;

PUBLIC static FIASCO_INIT
void
Cpu::init_tss_dbf (Address tss_dbf_mem, Address kdir)
{
  tss_dbf = reinterpret_cast<Tss*>(tss_dbf_mem);

  gdt->set_entry_byte (Gdt::gdt_tss_dbf/8, tss_dbf_mem, sizeof(Tss)-1,
		       Gdt_entry::Access_kernel | Gdt_entry::Access_tss |
		       Gdt_entry::Accessed, 0);

  tss_dbf->cs     = Gdt::gdt_code_kernel;
  tss_dbf->ss     = Gdt::gdt_data_kernel;
  tss_dbf->ds     = Gdt::gdt_data_kernel;
  tss_dbf->es     = Gdt::gdt_data_kernel;
  tss_dbf->fs     = Gdt::gdt_data_kernel;
  tss_dbf->gs     = Gdt::gdt_data_kernel;
  tss_dbf->eip    = (Address)entry_vec08_dbf;
  tss_dbf->esp    = (Address)&dbf_stack_top;
  tss_dbf->ldt    = 0;
  tss_dbf->eflags = 0x00000082;
  tss_dbf->cr3    = kdir;
  tss_dbf->io_bit_map_offset = 0x8000;
}

PUBLIC static inline NEEDS["gdt.h"]
void
Cpu::set_gdt ()
{
  Pseudo_descriptor desc((Address)gdt, Gdt::gdt_max-1);
  Gdt::set (&desc);
}

PUBLIC static inline NEEDS["gdt.h"]
void
Cpu::set_tss ()
{
  set_tr (Gdt::gdt_tss);
}

PUBLIC static inline
Gdt*
Cpu::get_gdt ()
{ return gdt; }

PUBLIC static inline
Tss*
Cpu::get_tss ()
{ return tss; }

PUBLIC static inline
void
Cpu::enable_ldt(Address addr, int size)
{
  if (!size)
    {
      get_gdt()->clear_entry (Gdt::gdt_ldt / 8);
      set_ldt(0);
    }
  else
    {
      get_gdt()->set_entry_byte (Gdt::gdt_ldt / 8, addr, size-1, 2/*=ldt*/, 0);
      set_ldt(Gdt::gdt_ldt);
    }
}
