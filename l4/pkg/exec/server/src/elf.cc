/* $Id$ */
/**
 * \file	exec/server/src/elf.cc
 * \brief	Some sharable functions for ELF handling
 *
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */

#include "elf.h"

unsigned long
elf_hash(const unsigned char *name)
{
  unsigned long h=0, g;
  while (*name)
    {
      h = (h << 4) + *name++;
      if ((g = h & 0xf0000000))
	h ^= g >> 24;
      h &= ~g;
    }
  return h;
}


