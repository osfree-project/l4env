#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <l4/util/elf.h>

#include "exec.h"

// our stdlib seems to have no alloca
//#ifndef ARCH_arm /* Just a reminder that dietlibc has it, oskit lacks it... */
//void
//*alloca(size_t size);
//#endif

int
exec_load_elf(exec_read_func_t *read, exec_read_exec_func_t *read_exec,
	      void *handle, const char **error_msg, l4_addr_t *entry)
{
  l4_size_t actual;
  MY_EHDR x;
  MY_PHDR *phdr, *ph;
  l4_size_t phsize;
  int i;
  int result;

  /* Read the ELF header.  */
  if ((result = (*read)(handle, 0, &x, sizeof(x), &actual)) != 0)
    return result;
  if (actual < sizeof(x))
    return *error_msg="no ELF executable", -1;

  if ((x.e_ident[EI_MAG0] != ELFMAG0) ||
      (x.e_ident[EI_MAG1] != ELFMAG1) ||
      (x.e_ident[EI_MAG2] != ELFMAG2) ||
      (x.e_ident[EI_MAG3] != ELFMAG3))
    return *error_msg="no ELF executable", -1;

  /* Make sure the file is of the right architecture.  */
  if ((x.e_ident[EI_CLASS] != MY_EI_CLASS) ||
      (x.e_ident[EI_DATA] != MY_EI_DATA) ||
      (x.e_machine != MY_E_MACHINE))
    return *error_msg="wrong ELF architecture", -1;

  *entry = (l4_addr_t) x.e_entry;

  phsize = x.e_phnum * x.e_phentsize;
  phdr = (MY_PHDR *)alloca(phsize);

  result = (*read)(handle, x.e_phoff, phdr, phsize, &actual);
  if (result)
    return result;
  if (actual < phsize)
    return *error_msg="ELF file corrupt", -1;

  for (i = 0; i < x.e_phnum; i++)
    {
      ph = (MY_PHDR *)((l4_addr_t)phdr + i * x.e_phentsize);
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

  return *error_msg="", 0;
}
