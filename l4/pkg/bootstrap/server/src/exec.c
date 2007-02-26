#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <alloca.h>

#include <l4/exec/elf.h>

#include "exec.h"

int
exec_load_elf(exec_read_func_t *read, exec_read_exec_func_t *read_exec,
	      void *handle, exec_info_t *out_info)
{
  l4_size_t actual;
  Elf32_Ehdr x;
  Elf32_Phdr *phdr, *ph;
  l4_size_t phsize;
  int i;
  int result;

  /* Read the ELF header.  */
  if ((result = (*read)(handle, 0, &x, sizeof(x), &actual)) != 0)
    return result;

  if (actual < sizeof(x))
    return EX_NOT_EXECUTABLE;

  if ((x.e_ident[EI_MAG0] != ELFMAG0) ||
      (x.e_ident[EI_MAG1] != ELFMAG1) ||
      (x.e_ident[EI_MAG2] != ELFMAG2) ||
      (x.e_ident[EI_MAG3] != ELFMAG3))
    return EX_NOT_EXECUTABLE;

  /* Make sure the file is of the right architecture.  */
  if ((x.e_ident[EI_CLASS] != ELFCLASS32) ||
      (x.e_ident[EI_DATA] != MY_EI_DATA) ||
      (x.e_machine != MY_E_MACHINE))
    return EX_WRONG_ARCH;

  /* XXX others */
  out_info->entry = (l4_addr_t) x.e_entry;

  phsize = x.e_phnum * x.e_phentsize;
  phdr = (Elf32_Phdr *)alloca(phsize);

  result = (*read)(handle, x.e_phoff, phdr, phsize, &actual);
  if (result)
    return result;

  if (actual < phsize)
    return EX_CORRUPT;

  for (i = 0; i < x.e_phnum; i++)
    {
      ph = (Elf32_Phdr *)((l4_addr_t)phdr + i * x.e_phentsize);
      if (ph->p_type == PT_LOAD)
	{
	  exec_sectype_t type = EXEC_SECTYPE_ALLOC |
				EXEC_SECTYPE_LOAD;
	  if (ph->p_flags & PF_R) type |= EXEC_SECTYPE_READ;
	  if (ph->p_flags & PF_W) type |= EXEC_SECTYPE_WRITE;
	  if (ph->p_flags & PF_X) type |= EXEC_SECTYPE_EXECUTE;
	  result = (*read_exec)(handle,
	            ph->p_offset, ph->p_filesz,
		    ph->p_paddr, ph->p_memsz, type);
	}
    }

  return 0;
}
