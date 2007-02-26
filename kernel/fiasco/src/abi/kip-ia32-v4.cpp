/*
 * IA-32 L4V4 Kernel-Info Page
 */

INTERFACE:

#include "types.h"

EXTENSION class Kernel_info
{
public:

  /* 00 */
  Mword magic;
  Mword version;
  Mword abi_flags;
  Address offset_version_strings;  

  /* 10 */
  Mword init_default_kdebug, 
    default_kdebug_exception, 
    padding00, 
    default_kdebug_end;

  /* 20 */
  Mword sigma0_esp, sigma0_eip;
  l4_low_high_t sigma0_memory;

  /* 30 */
  Mword sigma1_esp, sigma1_eip;
  l4_low_high_t sigma1_memory;
  
  /* 40 */
  Mword root_esp, root_eip;
  l4_low_high_t root_memory;

  /* 50 */
  Mword padding01;
  Mword mem_info_raw;
  Mword kdebug_config;
  Mword kdebug_permission;

  /* 60 */
  Mword padding02[16];

  /* A0 */
  Mword padding03[2];
  Mword utcb_info_raw;
  Mword kip_area_info;

  /* B0 */
  Mword padding04[2];
  Mword boot_info;
  Address processor_desc_ptr;

  /* C0 */
  Mword clock_info;
  Mword thread_info_raw;
  Mword page_info;
  Mword processor_info;

  /* D0 */
  Address sys_space_control;
  Address sys_thread_control;
  Address sys_processor_control;
  Address sys_memory_control;

  /* E0 */
  Address sys_ipc;
  Address sys_lipc;
  Address sys_fpage_unmap;
  Address sys_thread_ex_regs;

  /* F0 */
  Address sys_clock;
  Address sys_thread_switch;
  Address sys_thread_schedule;
  Mword unused_3;

  /* E0 */
  char version_strings[256];

  // processor description
  Mword frequency_bus;
  Mword frequency_cpu;

  //
  // entries to be removed
  //
  volatile Cpu_time clock; 
  //
  //

  char sys_calls[];
};


IMPLEMENTATION[ia32-v4]:

#include <cstdio>
#include "assert.h"
#include "l4_types.h"

/** Fill in the offset where the memory descriptors begin.
 * The count of memory descriptors is set to 0.
 */
PUBLIC
void Kernel_info::mem_info (Mword offset)
{ 
  mem_info_raw |= offset << 16; 
}

/** Add a memory descriptor.
 * @pre mem_info_raw initialized (call mem_info() or fill in the raw value)
 */
PUBLIC
void Kernel_info::mem_desc_add (Mword low, Mword high, Mword type,
				Mword subtype, Mword virt)
{
  Mword *p;

  // let the pointer point to the beginning of the first free 
  // memory descriptor field in the KIP
  p = (Mword*) ((Mword)this + mem_desc_offset()) + 2*mem_desc_count();
  
  // Make sure the descriptor is inside the KIP
  assert ((Mword)p >  (Mword)this);
  assert ((Mword)p <= (Mword)this + 4096);

  *p = type | (subtype<<4) | (virt ? 1<<9 : 0) | (low>>10<<10);

  p++; 
  *p = high >> 10 << 10;

  mem_info_raw++;
}

PUBLIC Mword Kernel_info::mem_desc_low	   (Mword n) const
{ 
  Mword *p = (Mword*) ((Mword)this + mem_desc_offset()) + 2*n;
  return *p >> 10 << 10; 
}

PUBLIC Mword Kernel_info::mem_desc_high    (Mword n) const
{ 
  Mword *p = (Mword*) ((Mword)this + mem_desc_offset()) + 2*n + 1;
  return *p >> 10 << 10; 
}

PUBLIC Mword Kernel_info::mem_desc_type    (Mword n) const
{ 
  Mword *p = (Mword*) ((Mword)this + mem_desc_offset()) + 2*n;
  return *p & 0x0000000F; 
}

PUBLIC Mword Kernel_info::mem_desc_subtype (Mword n) const
{ 
  Mword *p = (Mword*) ((Mword)this + mem_desc_offset()) + 2*n;
  return *p << 24 >> 28; 
}

PUBLIC Mword Kernel_info::mem_desc_v       (Mword n) const
{
  Mword *p = (Mword*) ((Mword)this + mem_desc_offset()) + 2*n;
  return *p << 22 >> 31; 
}

PUBLIC
Mword Kernel_info::mem_desc_count() const
{ 
  return ((Mword)mem_info_raw) << 16 >> 16; 
}

PRIVATE
Mword Kernel_info::mem_desc_offset() const
{ 
  return mem_info_raw >> 16; 
}

IMPLEMENT
Address Kernel_info::main_memory_high() const
{
  Address highest=0;
  
  // iterate over all memory descriptors
  for (Mword i=0; i<mem_desc_count(); i++)
    if (!mem_desc_v(i) &&		// phys-mem descriptors only
	mem_desc_type(i) == 1 &&	// conventional memory
	(highest < mem_desc_high(i))
	)
      highest = mem_desc_high(i);

  return highest;
}


// Returns 1 if the phys. memory address a belongs to a kernel-reserved
// or shared region.
PUBLIC
Mword Kernel_info::mem_is_reserved_or_shared (Address a) const
{
  // iterate over all memory descriptors, starting from the last one

  for (signed int i = mem_desc_count() - 1; i >= 0; i--)

    if (!mem_desc_v(i)			// phys-mem descriptors only
	&& mem_desc_low(i) <= a 	// does this desc. apply for a?
	&& mem_desc_high(i) > a
	&& ((mem_desc_type(i) == 2) || (mem_desc_type(i) == 4)))

      return 1;
  
  return 0;
}

PUBLIC inline
void Kernel_info::thread_info (Mword user_base, Mword system_base, 
			       Mword threadid_bits)
{
  thread_info_raw = (user_base << 20) | (system_base << 8) | threadid_bits;
}

IMPLEMENT inline
Mword const Kernel_info::max_threads() const
{
  return 1 << (thread_info_raw & ((1 << 8) - 1));
}

PUBLIC inline
void Kernel_info::utcb_info (Mword size, Mword align, Mword multiplier)
{
  utcb_info_raw = (size << 16) | (align << 10) | multiplier;
}

PUBLIC inline Mword Kernel_info::user_base() const
{ return thread_info_raw >> 20; }

PUBLIC inline Mword Kernel_info::system_base() const
{ return (thread_info_raw >> 8) & 0xFFF; }

PUBLIC inline Mword Kernel_info::threadid_bits() const
{ return thread_info_raw & 0xFF; }

IMPLEMENT
void Kernel_info::print() const
{
  char *memtypes[] = {
    "undefined",
    "conventional",
    "reserved",
    "dedicated",
    "shared", "5", "6", "7", "8", "9", "A", "B", "C", "D",
    "boot",
    "archdep"
  };

  printf("magic: %.4s  version: 0x%x\n",(char*)&magic,version);

  printf("root   mem:   [" L4_PTR_FMT "; " L4_PTR_FMT ")  "
	 "ESP:EIP  "L4_PTR_FMT" : "L4_PTR_FMT"\n"
	 "sigma0 mem:   [" L4_PTR_FMT "; " L4_PTR_FMT ")  "
	 "ESP:EIP  "L4_PTR_FMT" : "L4_PTR_FMT"\n"
	 "sigma1 mem:   [" L4_PTR_FMT "; " L4_PTR_FMT ")  "
	 "ESP:EIP  "L4_PTR_FMT" : "L4_PTR_FMT"\n",
	 L4_PTR_ARG(root_memory.low), L4_PTR_ARG(root_memory.high),
	 L4_PTR_ARG(root_esp), L4_PTR_ARG(root_eip),
	 L4_PTR_ARG(sigma0_memory.low), L4_PTR_ARG(sigma0_memory.high),
	 L4_PTR_ARG(sigma0_esp), L4_PTR_ARG(sigma0_eip),
	 L4_PTR_ARG(sigma1_memory.low), L4_PTR_ARG(sigma1_memory.high),
	 L4_PTR_ARG(sigma1_esp), L4_PTR_ARG(sigma1_eip));

  printf ("%d memory descriptors at offset " L4_PTR_FMT":\n",
	  mem_desc_count(), mem_desc_offset());
  
  for (Mword i=0; i < mem_desc_count(); i++) {
    printf ("  [" L4_PTR_FMT "; " L4_PTR_FMT ") %s %s.%d\n",
	    L4_PTR_ARG (mem_desc_low(i)), 
	    L4_PTR_ARG (mem_desc_high(i)),
	    mem_desc_v(i) ? "virt" : "phys",
	    memtypes[mem_desc_type(i)], 
	    mem_desc_subtype(i));
  }
  
  printf ("phys_mem_high = "L4_PTR_FMT"\n", main_memory_high());

  Mword 
    s_sc = sys_space_control,
    s_tc = sys_thread_control,
    s_pc = sys_processor_control,
    s_mc = sys_memory_control,
    s_ipc = sys_ipc, 
    s_lipc = sys_lipc,
    s_fpu = sys_fpage_unmap,
    s_tex = sys_thread_ex_regs,
    s_cl = sys_clock,
    s_tsw = sys_thread_switch, 
    s_tsc = sys_thread_schedule;
  
  printf("Syscall offsets:\n");
  printf("  space control: " L4_PTR_FMT 
	 "  thread control:" L4_PTR_FMT "\n"
	 "  proc. control: " L4_PTR_FMT 
	 "  memory control:" L4_PTR_FMT "\n"
	 "  ipc:           " L4_PTR_FMT 
	 "  lipc:          " L4_PTR_FMT "\n"
	 "  fpage unmap:   " L4_PTR_FMT 
	 "  ex regs:       " L4_PTR_FMT "\n"
	 "  clock:         " L4_PTR_FMT 
	 "  thread switch: " L4_PTR_FMT "\n"
	 "  schedule:      " L4_PTR_FMT "\n",
	 L4_PTR_ARG(s_sc ), L4_PTR_ARG(s_tc ), 
	 L4_PTR_ARG(s_pc ), L4_PTR_ARG(s_mc ), 
	 L4_PTR_ARG(s_ipc), L4_PTR_ARG(s_lipc),
	 L4_PTR_ARG(s_fpu), L4_PTR_ARG(s_tex),
	 L4_PTR_ARG(s_cl ), L4_PTR_ARG(s_tsw),
	 L4_PTR_ARG(s_tsc));

};
