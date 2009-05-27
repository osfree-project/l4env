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
    Lbr_uninitialized = 0,
    Lbr_unsupported,
    Lbr_pentium_6,
    Lbr_pentium_4,
    Lbr_pentium_4_ext,
  };

  enum Bts
  {
    Bts_uninitialized = 0,
    Bts_unsupported,
    Bts_pentium_m,
    Bts_pentium_4,
  };

private:
  static Unsigned32     debugctl_busy           asm ("CPU_DEBUGCTL_BUSY");
  static Unsigned32     debugctl_set            asm ("CPU_DEBUGCTL_SET");
  static Unsigned32     debugctl_reset		asm ("CPU_DEBUGCTL_RESET");
  static Lbr		lbr;
  static Bts            bts;
  static char           lbr_active;
  static char           btf_active;
  static char           bts_active;
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

Unsigned32     Cpu::debugctl_busy;
Unsigned32     Cpu::debugctl_set;
Unsigned32     Cpu::debugctl_reset;
Cpu::Lbr       Cpu::lbr;
Cpu::Bts       Cpu::bts;
char           Cpu::lbr_active;
char           Cpu::btf_active;
char           Cpu::bts_active;
Cpu::Sysenter *Cpu::current_sysenter;
Gdt           *Cpu::gdt;
Tss           *Cpu::tss;
Tss           *Cpu::tss_dbf;

extern "C" void entry_sys_fast_ipc (void);
extern "C" void entry_sys_fast_ipc_c (void);


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
{ asm volatile ("movl %0, %%ss" : : "r" (val)); }

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
Cpu::Lbr
Cpu::lbr_type()
{
  if (lbr == Lbr_uninitialized)
    {
      lbr = Lbr_unsupported;

      if (can_wrmsr())
	{
	  // Intel
	  if (vendor() == Vendor_intel)
	    {
	      if (family() == 15)
		lbr = model() < 3 ? Lbr_pentium_4 : Lbr_pentium_4_ext; // P4
	      else if (family() >= 6)
		lbr = Lbr_pentium_6; // PPro, PIII
	    }
	  else if (vendor() == Vendor_amd)
	    {
	      if ((family() == 6) || (family() == 15))
		lbr = Lbr_pentium_6; // K7/K8
	    }
	}
    }

  return lbr;
}

PUBLIC static inline
bool
Cpu::lbr_status ()
{ return lbr_active; }

PUBLIC static inline
void
Cpu::lbr_enable (bool on)
{
  if (lbr_type() != Lbr_unsupported)
    {
      if (on)
	{
	  lbr_active    = true;
	  debugctl_set |= 1;
	  debugctl_busy = true;
	}
      else
	{
	  lbr_active    = false;
	  debugctl_set &= ~1;
	  debugctl_busy = lbr_active || bts_active;
	  wrmsr(debugctl_reset, MSR_DEBUGCTLA);
	}
    }
}

PUBLIC static inline
bool
Cpu::btf_status ()
{ return btf_active; }

PUBLIC static inline
void
Cpu::btf_enable (bool on)
{
  if (lbr_type() != Lbr_unsupported)
    {
      if (on)
	{
	  btf_active      = true;
	  debugctl_set   |= 2;
	  debugctl_reset |= 2; /* don't disable bit in kernel */
	  wrmsr(2, MSR_DEBUGCTLA);     /* activate _now_ */
	}
      else
	{
	  btf_active    = false;
	  debugctl_set &= ~2;
	  debugctl_busy = lbr_active || bts_active;
	  wrmsr(debugctl_reset, MSR_DEBUGCTLA);
	}
    }
}

PUBLIC static
Cpu::Bts
Cpu::bts_type ()
{
  if (bts == Bts_uninitialized)
    {
      bts = Bts_unsupported;

      if (can_wrmsr() && vendor() == Vendor_intel)
	{
	  if (family() == 15 && (rdmsr(0x1A0) & (1<<11)) == 0)
	    bts = Bts_pentium_4;
	  if (family() == 6  && (model() == 9 || (model() >= 13 &&
						  model() <= 15)))
	    bts = Bts_pentium_m;
	  if (!(features() & FEAT_DS))
	    bts = Bts_unsupported;
	}
    }

  return bts;
}

PUBLIC static inline
bool
Cpu::bts_status ()
{ return bts_active; }

PUBLIC static
void
Cpu::bts_enable (bool on)
{
  if (bts_type() != Bts_unsupported)
    {
      if (on)
	{
	  switch (bts_type())
	    {
	    case Bts_pentium_4: bts_active = true; debugctl_set |= 0x0c; break;
	    case Bts_pentium_m: bts_active = true; debugctl_set |= 0xc0; break;
	    default:;
	    }
	  debugctl_busy = lbr_active || bts_active;
	}
      else
	{
	  bts_active = false;
	  switch (bts_type())
	    {
	    case Bts_pentium_4: debugctl_set &= ~0x0c; break;
	    case Bts_pentium_m: debugctl_set &= ~0xc0; break;
	    default:;
	    }
	  debugctl_busy = lbr_active || bts_active;
	  wrmsr(debugctl_reset, MSR_DEBUGCTLA);
	}
    }
}

PUBLIC static inline
void
Cpu::debugctl_enable ()
{
  if (debugctl_busy)
    wrmsr(debugctl_set, MSR_DEBUGCTLA);
}

PUBLIC static inline
void
Cpu::debugctl_disable ()
{
  if (debugctl_busy)
    wrmsr(debugctl_reset, MSR_DEBUGCTLA);
}

IMPLEMENT FIASCO_INIT
void
Cpu::init ()
{
  identify();

  calibrate_tsc();

  if (scaler_tsc_to_ns)
    _frequency = ns_to_tsc(1000000000UL);

  Unsigned32 cr4 = get_cr4();

  if (features() & FEAT_FXSR)
    cr4 |= CR4_OSFXSR;

  if (features() & FEAT_SSE)
    cr4 |= CR4_OSXMMEXCPT;

  set_cr4 (cr4);

  // reset time stamp counter (better for debugging)
  if ((features() & FEAT_TSC) && can_wrmsr())
    wrmsr(0, 0, 0x10);

  if ((features() & FEAT_PAT) && can_wrmsr())
    wrmsr(0x00010406, 0x00070406, 0x277);
}

PUBLIC static FIASCO_INIT
void
Cpu::init_sysenter (Address kernel_esp)
{
  // Check for Sysenter/Sysexit Feature
  if (have_sysenter())
    {
      wrmsr (Gdt::gdt_code_kernel, 0, MSR_SYSENTER_CS);
      wrmsr ((Unsigned32) kernel_esp, 0, MSR_SYSENTER_ESP);
      if (Config::Assembler_ipc_shortcut)
	set_sysenter(entry_sys_fast_ipc);
      else
	set_sysenter(entry_sys_fast_ipc_c);
    }
}

PUBLIC static
void
Cpu::set_sysenter (void (*func)(void))
{
  // Check for Sysenter/Sysexit Feature
  if (have_sysenter())
    {
      wrmsr ((Unsigned32) func, 0, MSR_SYSENTER_EIP);
      current_sysenter = func;
    }
}

PUBLIC static
void
Cpu::get_sysenter (void (**func)(void))
{
  *func = current_sysenter;
}

PUBLIC static
void
Cpu::set_fast_entry(void (*func)(void))
{
  set_sysenter(func);
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

  tss->_ss0 = Gdt::gdt_data_kernel;
  tss->_io_bit_map_offset = Mem_layout::Io_bitmap - tss_mem;
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

  tss_dbf->_cs     = Gdt::gdt_code_kernel;
  tss_dbf->_ss     = Gdt::gdt_data_kernel;
  tss_dbf->_ds     = Gdt::gdt_data_kernel;
  tss_dbf->_es     = Gdt::gdt_data_kernel;
  tss_dbf->_fs     = Gdt::gdt_data_kernel;
  tss_dbf->_gs     = Gdt::gdt_data_kernel;
  tss_dbf->_eip    = (Address)entry_vec08_dbf;
  tss_dbf->_esp    = (Address)&dbf_stack_top;
  tss_dbf->_ldt    = 0;
  tss_dbf->_eflags = 0x00000082;
  tss_dbf->_cr3    = kdir;
  tss_dbf->_io_bit_map_offset = 0x8000;
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


PUBLIC static inline
Unsigned32
Cpu::get_gs()
{ Unsigned32 val; asm volatile ("mov %%gs, %0" : "=rm" (val)); return val; }

PUBLIC static inline
void
Cpu::set_gs (Unsigned32 val)
{ asm volatile ("mov %0, %%gs" : : "rm" (val)); }

