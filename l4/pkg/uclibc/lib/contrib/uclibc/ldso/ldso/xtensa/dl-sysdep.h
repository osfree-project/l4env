/* Machine-dependent ELF dynamic relocation.
   Parts copied from glibc/sysdeps/xtensa/dl-machine.h
   Copyright (C) 2001, 2007 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* Define this if the system uses RELOCA.  */
#define ELF_USES_RELOCA
#include <elf.h>
#include <link.h>

/* Translate a processor specific dynamic tag to the index
   in l_info array.  */
#define DT_XTENSA(x) (DT_XTENSA_##x - DT_LOPROC + DT_NUM + OS_NUM)

typedef struct xtensa_got_location_struct {
  Elf32_Off offset;
  Elf32_Word length;
} xtensa_got_location;

/* Initialization sequence for the GOT.  */
#define INIT_GOT(GOT_BASE, MODULE) \
  do {									      \
    xtensa_got_location *got_loc;					      \
    Elf32_Addr l_addr = MODULE->loadaddr;				      \
    int x;								      \
									      \
    got_loc = (xtensa_got_location *)					      \
      (MODULE->dynamic_info[DT_XTENSA (GOT_LOC_OFF)] + l_addr);		      \
									      \
    for (x = 0; x < MODULE->dynamic_info[DT_XTENSA (GOT_LOC_SZ)]; x++)	      \
      {									      \
	Elf32_Addr got_start, got_end;					      \
	got_start = got_loc[x].offset & ~(PAGE_SIZE - 1);		      \
	got_end = ((got_loc[x].offset + got_loc[x].length + PAGE_SIZE - 1)    \
		   & ~(PAGE_SIZE - 1));					      \
	_dl_mprotect ((void *)(got_start + l_addr) , got_end - got_start,     \
		      PROT_READ | PROT_WRITE | PROT_EXEC);		      \
      }									      \
									      \
    /* Fill in first GOT entry according to the ABI.  */		      \
    GOT_BASE[0] = (unsigned long) _dl_linux_resolve;			      \
  } while (0)

/* Parse dynamic info */
#define ARCH_NUM 2
#define ARCH_DYNAMIC_INFO(dpnt, dynamic, debug_addr) \
  do {									\
    if (dpnt->d_tag == DT_XTENSA_GOT_LOC_OFF)				\
      dynamic[DT_XTENSA (GOT_LOC_OFF)] = dpnt->d_un.d_ptr;		\
    else if (dpnt->d_tag == DT_XTENSA_GOT_LOC_SZ)			\
      dynamic[DT_XTENSA (GOT_LOC_SZ)] = dpnt->d_un.d_val;		\
  } while (0)

/* Here we define the magic numbers that this dynamic loader should accept. */
#define MAGIC1 EM_XTENSA
#undef	MAGIC2

/* Used for error messages. */
#define ELF_TARGET "Xtensa"

struct elf_resolve;
extern unsigned long _dl_linux_resolver (struct elf_resolve *, int);

/* 4096 bytes alignment */
#define PAGE_ALIGN 0xfffff000
#define ADDR_ALIGN 0xfff
#define OFFS_ALIGN 0x7ffff000

/* ELF_RTYPE_CLASS_PLT iff TYPE describes relocation of a PLT entry, so
   undefined references should not be allowed to define the value.  */
#define elf_machine_type_class(type) \
  (((type) == R_XTENSA_JMP_SLOT) * ELF_RTYPE_CLASS_PLT)

/* Return the link-time address of _DYNAMIC.  */
static inline Elf32_Addr
elf_machine_dynamic (void)
{
  /* This function is only used while bootstrapping the runtime linker.
     The "_DYNAMIC" symbol is always local so its GOT entry will initially
     contain the link-time address.  */
  return (Elf32_Addr) &_DYNAMIC;
}

/* Return the run-time load address of the shared object.  */
static inline Elf32_Addr
elf_machine_load_address (void)
{
  Elf32_Addr addr, tmp;

  /* At this point, the runtime linker is being bootstrapped and the GOT
     entry used for ".Lhere" will contain the link address.  The CALL0 will
     produce the dynamic address of ".Lhere" + 3.  Thus, the end result is
     equal to "dynamic_address(.Lhere) - link_address(.Lhere)".  */
  __asm__ ("\
	movi	%0, .Lhere\n\
	mov	%1, a0\n\
.Lhere:	_call0	0f\n\
	.align	4\n\
0:	sub	%0, a0, %0\n\
	mov	a0, %1"
	   : "=a" (addr), "=a" (tmp));

  return addr - 3;
}

static inline void
elf_machine_relative (Elf32_Addr load_off, const Elf32_Addr rel_addr,
		      Elf32_Word relative_count)
{
  Elf32_Rela *rpnt = (Elf32_Rela *) rel_addr;
  while (relative_count--)
    {
      Elf32_Addr *const reloc_addr = (Elf32_Addr *) (load_off + rpnt->r_offset);
      *reloc_addr += load_off + rpnt->r_addend;
      rpnt++;
    }
}
