/* $Id$ */
/**
 * \file	exec/server/src/elf.cc
 * \brief	Some sharable functions for ELF handling
 *
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include "elf.h"

/** Determine ELF hash value from symbol name as described in ELF standard */
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


