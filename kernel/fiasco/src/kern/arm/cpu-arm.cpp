INTERFACE [arm]:

#include "types.h"

class Cpu
{
public:
  static void early_init();
  static void init();

  enum {
    Cp15_c1_mmu             = 1 << 0,
    Cp15_c1_alignment_check = 1 << 1,
    Cp15_c1_cache           = 1 << 2,
    Cp15_c1_branch_predict  = 1 << 11,
    Cp15_c1_insn_cache      = 1 << 12,
    Cp15_c1_high_vector     = 1 << 13,
    Cp15_c1_l4              = 1 << 15,
  };
};

INTERFACE [arm && armv5]:

EXTENSION class Cpu
{
public:
  enum {
    Cp15_c1_write_buffer    = 1 << 3,
    Cp15_c1_prog32          = 1 << 4,
    Cp15_c1_data32          = 1 << 5,
    Cp15_c1_late_abort      = 1 << 6,
    Cp15_c1_big_endian      = 1 << 7,
    Cp15_c1_system_protect  = 1 << 8,
    Cp15_c1_rom_protect     = 1 << 9,
    Cp15_c1_f               = 1 << 10,
    Cp15_c1_rr              = 1 << 14,

    Cp15_c1_generic         = Cp15_c1_mmu
                              | Cp15_c1_alignment_check
                              | Cp15_c1_write_buffer
                              | Cp15_c1_prog32
                              | Cp15_c1_data32
                              | Cp15_c1_late_abort
                              | Cp15_c1_rom_protect
                              | Cp15_c1_high_vector,

    Cp15_c1_cache_bits      = Cp15_c1_cache
                              | Cp15_c1_insn_cache
                              | Cp15_c1_write_buffer,

  };
};

INTERFACE [arm && armv6]:

EXTENSION class Cpu
{
public:
  enum {
    Cp15_c1_u               = 1 << 22,
    Cp15_c1_xp              = 1 << 23,
    Cp15_c1_ee              = 1 << 25,
    Cp15_c1_nmfi            = 1 << 27,
    Cp15_c1_tex             = 1 << 28,
    Cp15_c1_force_ap        = 1 << 29,

    Cp15_c1_generic         = Cp15_c1_mmu
                              | Cp15_c1_alignment_check
			      | Cp15_c1_branch_predict
			      | Cp15_c1_high_vector
			      | Cp15_c1_xp,

    Cp15_c1_cache_bits      = Cp15_c1_cache
                              | Cp15_c1_insn_cache,
  };
};

INTERFACE [arm && armv7]:

EXTENSION class Cpu
{
public:
  enum {
    Cp15_c1_ee              = 1 << 25,
    Cp15_c1_nmfi            = 1 << 27,
    Cp15_c1_tre             = 1 << 28,
    Cp15_c1_afe             = 1 << 29,
    Cp15_c1_te              = 1 << 30,

    Cp15_c1_cache_bits      = Cp15_c1_cache
                              | Cp15_c1_insn_cache,

    Cp15_c1_generic         = Cp15_c1_mmu
                              | Cp15_c1_alignment_check
			      | Cp15_c1_high_vector
			      | Cp15_c1_xp,
  };
};

INTERFACE [arm]:

EXTENSION class Cpu
{
public:
  enum {
    Cp15_c1_cache_enabled  = Cp15_c1_generic | Cp15_c1_cache_bits,
    Cp15_c1_cache_disabled = Cp15_c1_generic,
  };
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <cstdio>
#include <cstring>
#include <panic.h>

#include "pagetable.h"
#include "kmem_space.h"
#include "mem_unit.h"
#include "ram_quota.h"

IMPLEMENT
void Cpu::early_init()
{
  // switch to supervisor mode and intialize the memory system
  asm volatile ( " mov  r2, r13             \n"
		 " mov  r3, r14             \n"
		 " msr  cpsr_c, %1          \n"
		 " mov  r13, r2             \n"
		 " mov  r14, r3             \n"

		 " mcr  p15, 0, %0, c1, c0   \n"
		 :
		 :
		 "r"(Config::cache_enabled
                     ? Cp15_c1_cache_enabled : Cp15_c1_cache_disabled),
		 "I"(0x0d3)
		 : "r2","r3"
		 );

  Mem_unit::flush_cache();
}


PUBLIC static inline
bool
Cpu::have_superpages()
{ return true; }

PUBLIC static inline
void
Cpu::debugctl_enable()
{}

PUBLIC static inline
void
Cpu::debugctl_disable()
{}

PUBLIC static inline NEEDS["types.h"]
Unsigned32
Cpu::get_scaler_tsc_to_ns()
{ return 0; }

PUBLIC static inline NEEDS["types.h"]
Unsigned32
Cpu::get_scaler_tsc_to_us()
{ return 0; }

PUBLIC static inline NEEDS["types.h"]
Unsigned32
Cpu::get_scaler_ns_to_tsc()
{ return 0; }

PUBLIC static inline
bool
Cpu::have_tsc()
{ return 0; }

PUBLIC static inline
Unsigned64
Cpu::rdtsc (void)
{ return 0; }

IMPLEMENT
void Cpu::init()
{
  extern char ivt_start;
  // map the interrupt vector table to 0xffff0000
  Pte pte = Kmem_space::kdir()->walk((void*)Kmem_space::Ivt_base, 4096,
      true, Ram_quota::root);

  pte.set((unsigned long)&ivt_start, 4096, 
      Mem_page_attr(Page::KERN_RW | Page::CACHEABLE),
      true);

  Mem_unit::tlb_flush();
}


PUBLIC inline static
void
Cpu::memcpy_mwords( void *dst, void const *src, unsigned words)
{
  __builtin_memcpy(dst, src, words * sizeof(unsigned long));
}
