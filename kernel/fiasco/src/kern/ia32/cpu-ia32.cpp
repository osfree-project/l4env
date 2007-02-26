/*
 * Fiasco ia32-native
 * Architecture specific cpu init code
 */

INTERFACE:

#include "types.h"

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

  static void		init_sysenter(Address kernel_esp);
  static void		set_sysenter(void (*do_sysenter)(void));
  static Unsigned64	rdtsc();
  static Unsigned64	rdmsr(Unsigned32 reg);
  static Unsigned64	rdpmc(Unsigned32 reg);
  static void		wrmsr(Unsigned32 low, Unsigned32 high, Unsigned32 reg);
  static void		wrmsr(Unsigned64 msr, Unsigned32 reg);
  static Unsigned32	hz();
  static Unsigned64	tsc_to_ns(Unsigned64 tsc);
  static Unsigned64	ns_to_tsc(Unsigned64 ns);
  static Unsigned32	get_scaler_tsc_to_ns();
  static Unsigned32	get_scaler_tsc_to_us();
  static Unsigned32	get_scaler_ns_to_tsc();
  static void		enable_lbr();
  static void		disable_lbr();
  static Lbr const      lbr_type();
  static void		tsc_stop();
  static void		tsc_continue();

private:
  static Lbr cpu_lbr;
  static Unsigned32 scaler_tsc_to_ns;
  static Unsigned32 scaler_tsc_to_us;
  static Unsigned32 scaler_ns_to_tsc;
  static Unsigned64 tsc_correction;

  static void calibrate_tsc();
};

IMPLEMENTATION[ia32]:

#include <cstdio>
#include <flux/x86/proc_reg.h>
#include "boot_info.h"
#include "config.h"
#include "config_gdt.h"
#include "globals.h"
#include "initcalls.h"
#include "io.h"
#include "pit.h"
#include "processor.h"
#include "regdefs.h"

Cpu::Lbr Cpu::cpu_lbr;

Unsigned32 Cpu::scaler_tsc_to_ns;
Unsigned32 Cpu::scaler_tsc_to_us;
Unsigned32 Cpu::scaler_ns_to_tsc;
Unsigned64 Cpu::tsc_correction;

extern "C" void do_sysenter (void);
extern "C" void do_sysenter_c (void);

#ifdef BOCHS
PRIVATE static
void
Cpu::bochs_tweaks()
{
  multiboot_info *kmbi = reinterpret_cast<multiboot_info *>
						(boot_info::mbi_phys());

  if (kmbi->flags & MULTIBOOT_CMDLINE &&
      strstr (reinterpret_cast<char *>(kmbi->cmdline), " -bochs")) {

    puts ("configuring bochs compatibility");

    config::hlt_works_ok	= false;
    config::getchar_does_hlt	= false;
    config::pic_prio_modify	= false;

    cpu_features &= ~FEAT_TSC;		// CPUID is broken!
  }
}
#endif

IMPLEMENT inline
Unsigned64
Cpu::rdtsc (void)
{
  Unsigned64 tsc;

  asm volatile ("rdtsc" : "=A" (tsc));
  return tsc;
}

IMPLEMENT inline
Unsigned64
Cpu::rdmsr (Unsigned32 reg)
{
  Unsigned64 msr;

  asm volatile ("rdmsr" : "=A" (msr) : "c" (reg));
  return msr;
}

IMPLEMENT inline
Unsigned64
Cpu::rdpmc (Unsigned32 reg)
{
  Unsigned64 pmc;

  asm volatile ("rdpmc" : "=A" (pmc) : "c" (reg));
  return pmc;
}

IMPLEMENT inline
void
Cpu::wrmsr (Unsigned32 low, Unsigned32 high, Unsigned32 reg)
{
  asm volatile ("wrmsr" : : "a" (low), "d" (high), "c" (reg));
}

IMPLEMENT inline
void
Cpu::wrmsr (Unsigned64 msr, Unsigned32 reg)
{
  asm volatile ("wrmsr" : : "A" (msr), "c" (reg));
}

IMPLEMENT
Cpu::Lbr const
Cpu::lbr_type()
{
  if (cpu_lbr == LBR_UNINITIALIZED)
    {
      cpu_lbr = LBR_NONE;

      if (features() & FEAT_MSR)
	{
	  // Intel
	  if (vendor() == VENDOR_INTEL)
	    {
	      if (family() == 15)
		cpu_lbr = LBR_P4; // P4
	      else if (family() >= 6)
		cpu_lbr = LBR_P6; // PPro, PIII
	    }
	  else if (vendor() == VENDOR_AMD)
	    {
	      if (family() == 6)
		cpu_lbr = LBR_P6; // K7
	    }
	}
    }

  return cpu_lbr;
}

IMPLEMENT inline
void
Cpu::enable_lbr (void)
{
  if (lbr_type() != LBR_NONE)
    {
      Unsigned64 lbr_ctrl = rdmsr (0x1d9);
      wrmsr (lbr_ctrl | 1, 0x1d9);
    }
}

IMPLEMENT inline
void
Cpu::disable_lbr (void)
{
  if (lbr_type() != LBR_NONE)
    {
      Unsigned64 lbr_ctrl = rdmsr (0x1d9);
      wrmsr (lbr_ctrl & ~1, 0x1d9);
    }
}

IMPLEMENT inline NEEDS["regdefs.h", "config.h"]
void
Cpu::tsc_stop (void)
{
  if (Config::kinfo_timer_uses_rdtsc && (features() & FEAT_TSC))
    tsc_correction += rdtsc();
}

IMPLEMENT inline NEEDS["config.h"]
void
Cpu::tsc_continue (void)
{
  if (Config::kinfo_timer_uses_rdtsc && (features() & FEAT_TSC))
    {
      // on Intel processors, the high-order 32 bits are cleared to 0s
      wrmsr(tsc_correction & 0xffffffff, 0, 0x10);
      tsc_correction &= 0xffffffff00000000ULL;
    }
}

IMPLEMENT FIASCO_INIT
void
Cpu::init (void)
{
  identify();

  calibrate_tsc();

#ifdef BOCHS
  bochs_tweaks();
#endif

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

IMPLEMENT FIASCO_INIT
void
Cpu::init_sysenter (Address kernel_esp)
{
  // Check for Sysenter/Sysexit Feature
  if (features() & FEAT_SEP)
    {
      wrmsr (GDT_CODE_KERNEL, 0, SYSENTER_CS_MSR);
      wrmsr ((Unsigned32) kernel_esp, 0, SYSENTER_ESP_MSR);
#if !defined(CONFIG_ASSEMBLER_IPC_SHORTCUT) || defined(CONFIG_PROFILE)
      wrmsr ((Unsigned32) do_sysenter_c, 0, SYSENTER_EIP_MSR);
#else
      wrmsr ((Unsigned32) do_sysenter, 0, SYSENTER_EIP_MSR);
#endif
    }
}

IMPLEMENT
void
Cpu::set_sysenter (void (*do_sysenter)(void))
{
  // Check for Sysenter/Sysexit Feature
  if (features() & FEAT_SEP)
    wrmsr ((Unsigned32) do_sysenter, 0, SYSENTER_EIP_MSR);
}

static inline
unsigned long
Cpu::muldiv (unsigned long a, unsigned long mul, unsigned long div)
{
  asm volatile ("mull %1 ; divl %2\n\t"
               :"=a" (a)
               :"d" (mul),
                "c" (div),
                "0" (a)
               );
  return a;
}

// Return 2^32 / (tsc clocks per usec)
IMPLEMENT FIASCO_INIT
void
Cpu::calibrate_tsc (void)
{
#define CLOCK_TICK_RATE 	1193180
#define CALIBRATE_TIME  	50001
#define CALIBRATE_LATCH		(CLOCK_TICK_RATE / 20) // 50 ms

  // sanity check
  if (! (features() & FEAT_TSC))
    goto bad_ctc;

  Pit::setup_channel2_to_200hz();

  Unsigned64 tsc_start, tsc_end;
  Unsigned32 count, tsc_to_ns_div, dummy;

    {
      // disable interrupts
      Proc::Status o = Proc::cli_save();

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

  // Error: ECPUTOOFAST
  if (tsc_end & 0xffffffff00000000LL)
    goto bad_ctc;

  // Error: ECPUTOOSLOW
  if ((tsc_end & 0xffffffffL) < CALIBRATE_TIME)
    goto bad_ctc;

  asm volatile ("divl %2"
		:"=a" (tsc_to_ns_div), "=d" (dummy)
		:"r" ((Unsigned32)tsc_end), "0" (0), "1" (CALIBRATE_TIME));

  scaler_tsc_to_ns = muldiv (tsc_to_ns_div, 1000, 1<<5);
  scaler_tsc_to_us = muldiv (tsc_to_ns_div,    1, 1<<5);
  scaler_ns_to_tsc = muldiv (((1ULL<<32)/1000ULL), ((Unsigned32)tsc_end),
			       CALIBRATE_TIME * (1<<5));

  return;

bad_ctc:
  if (Config::kinfo_timer_uses_rdtsc)
    panic("Can't calibrate tsc");
}

IMPLEMENT inline
Unsigned64
Cpu::ns_to_tsc (Unsigned64 ns)
{
  Unsigned32 dummy;
  Unsigned64 tsc;
  asm volatile
	("movl  %%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	:"=A" (tsc), "=c" (dummy)
	: "0" (ns),  "b" (scaler_ns_to_tsc)
	);
  return tsc;
}

IMPLEMENT inline
Unsigned64
Cpu::tsc_to_ns (Unsigned64 tsc)
{
  Unsigned32 dummy;
  Unsigned64 ns;
  asm volatile
	("movl  %%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	:"=A" (ns), "=c" (dummy)
	: "0" (tsc), "b" (scaler_tsc_to_ns)
	);
  return ns;
}

IMPLEMENT inline
Unsigned64
Cpu::tsc_to_us (Unsigned64 tsc)
{
  Unsigned32 dummy;
  Unsigned64 us;
  asm volatile
	("movl  %%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	:"=A" (us), "=c" (dummy)
	: "0" (tsc), "S" (scaler_tsc_to_us)
	);
  return us;
}

IMPLEMENT
Unsigned32
Cpu::hz (void)
{
  if (!scaler_tsc_to_ns)
    return 0;

  return (Unsigned32)(ns_to_tsc(1000000000UL));
}

IMPLEMENT inline
Unsigned32
Cpu::get_scaler_tsc_to_ns()
{
  return scaler_tsc_to_ns;
}

IMPLEMENT inline
Unsigned32
Cpu::get_scaler_tsc_to_us()
{
  return scaler_tsc_to_us;
}

IMPLEMENT inline
Unsigned32
Cpu::get_scaler_ns_to_tsc()
{
  return scaler_ns_to_tsc;
}

IMPLEMENT inline
Unsigned64
Cpu::time_us (void)
{
  return tsc_to_us ((tsc_correction & 0xffffffff00000000ULL) + rdtsc());
}
