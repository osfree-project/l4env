/* $Id$ */
/**
 * \file	bootstrap/server/src/exec.c
 * \brief	ELF loader
 * 
 * \date	2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *		Torsten Frenzel <frenzel@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <alloca.h>

#include <l4/util/elf.h>

#include "exec.h"

int
exec_load_elf(exec_handler_func_t *handler,
	      void *handle, const char **error_msg, l4_addr_t *entry)
{
  exec_task_t *t = handle;
  MY_EHDR *x = t->mod_start;
  MY_PHDR *phdr, *ph;
  int i;
  int result;

  /* Read the ELF header.  */

  if ((x->e_ident[EI_MAG0] != ELFMAG0) ||
      (x->e_ident[EI_MAG1] != ELFMAG1) ||
      (x->e_ident[EI_MAG2] != ELFMAG2) ||
      (x->e_ident[EI_MAG3] != ELFMAG3))
    return *error_msg="no ELF executable", -1;

  /* Make sure the file is of the right architecture.  */
  if ((x->e_ident[EI_CLASS] != MY_EI_CLASS) ||
      (x->e_ident[EI_DATA] != MY_EI_DATA) ||
      (x->e_machine != MY_E_MACHINE))
    return *error_msg="wrong ELF architecture", -1;

  *entry = (l4_addr_t) x->e_entry;

  phdr   = (MY_PHDR*)(((char*)x) + x->e_phoff);

  for (i = 0; i < x->e_phnum; i++)
    {
      ph = (MY_PHDR *)((l4_addr_t)phdr + i * x->e_phentsize);
      if (ph->p_type == PT_LOAD)
	{
	  exec_sectype_t type = EXEC_SECTYPE_ALLOC |
	    EXEC_SECTYPE_LOAD;
	  if (ph->p_flags & PF_R) type |= EXEC_SECTYPE_READ;
	  if (ph->p_flags & PF_W) type |= EXEC_SECTYPE_WRITE;
	  if (ph->p_flags & PF_X) type |= EXEC_SECTYPE_EXECUTE;
	  result = (*handler)(handle,
	            ph->p_offset, ph->p_filesz,
		    ph->p_paddr, ph->p_vaddr, ph->p_memsz, type);
	}
    }

  return *error_msg="", 0;
}
