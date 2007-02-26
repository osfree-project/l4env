/**
 * \file	roottask/server/src/symbols.h
 * \brief	extract symbols information from ELF binary
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#ifndef SYMBOLS_H
#define SYMBOLS_H

#ifdef ARCH_x86
#define OFFSET 11
#endif

#ifdef ARCH_amd64
#define OFFSET 19
#endif

void
extract_symbols_from_image(l4_addr_t elf_image, unsigned task_no,
			   l4_addr_t *from_sym, l4_addr_t *to_sym);

void
extract_symbols_from_mbinfo(l4util_mb_info_t *mbi, unsigned task_no,
			    l4_addr_t *from_sym, l4_addr_t *to_sym);

#endif
