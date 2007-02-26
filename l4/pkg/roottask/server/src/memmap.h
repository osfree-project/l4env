/**
 * \file	roottask/server/src/memmap.h
 * \brief	memory resource handling
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 */
#ifndef MEMMAP_H
#define MEMMAP_H

#if defined(ARCH_arm)
#define MEM_MAX		(1024 << 20)	// 1GB
#define PAGE_MAX	(MEM_MAX / 4096)
#define SUPERPAGE_MAX	(4 << 10)
#elif defined(ARCH_x86)
#define MEM_MAX		(0)		// 4GB
#define PAGE_MAX	(1 << 20)
#define SUPERPAGE_MAX	(1 << 10)
#elif defined(ARCH_amd64)
#define MEM_MAX		(0x100000000UL)	// 4GB
#define PAGE_MAX	(1 << 20)
#define SUPERPAGE_MAX	(2 << 10)
#else
#error Invalid architecture
#endif


#ifndef __ASSEMBLER__

#include <l4/sys/types.h>
#include "types.h"

enum
{
  ram_base = RAM_BASE,
};

extern owner_t __memmap[];

void     memmap_init(void);
void     memmap_set(unsigned start, int state, unsigned size);
void     memmap_set_range(l4_addr_t start, l4_addr_t end, owner_t owner);
int      memmap_set_page(l4_addr_t address, owner_t owner);
int      memmap_free_page(l4_addr_t address, owner_t owner);
int      memmap_alloc_page(l4_addr_t address, owner_t owner);
owner_t  memmap_owner_page(l4_addr_t address);
int      memmap_free_superpage(l4_addr_t address, owner_t owner);
int      memmap_alloc_superpage(l4_addr_t address, owner_t owner);
owner_t  memmap_owner_superpage(l4_addr_t address);
unsigned memmap_nrfree_superpage(l4_addr_t address);
void     memmap_dump(void);

l4_addr_t find_free_chunk(l4_umword_t pages, int down);
l4_addr_t reserve_range(unsigned int size_and_align, owner_t owner,
			l4_addr_t range_low, l4_addr_t range_high);

extern l4_addr_t mem_high;

#endif /* __ASSEMBLER__ */

#endif
