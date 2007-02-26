#include <stdio.h>
#include <string.h>

#include "types.h"
#include <l4/util/elf.h>
#include "load_elf.h"

l4_uint32_t
load_elf (void *elf, l4_uint32_t *vma_start, l4_uint32_t *vma_end)
{
  char *_elf = (char *) elf;
  Elf64_Ehdr *eh = (Elf64_Ehdr *)(_elf);
  Elf64_Phdr *ph = (Elf64_Phdr *)(_elf + eh->e_phoff);
  l4_uint32_t _vma_start = ~0, _vma_end = 0;
  int i;

  for (i = 0; i < eh->e_phnum; i++, ph++) {
    if (ph->p_type != PT_LOAD)
      continue;

    if (ph->p_vaddr < _vma_start)
      _vma_start = ph->p_vaddr;
    
    if (ph->p_vaddr + ph->p_memsz > _vma_end)
      _vma_end = ph->p_vaddr + ph->p_memsz;

    memcpy ((void*)((Elf32_Addr)ph->p_paddr), 
	    _elf + ph->p_offset, ph->p_filesz);

    if (ph->p_filesz < ph->p_memsz)
      memset ((void*) ((Elf32_Addr)(ph->p_paddr + ph->p_filesz)), 0,
                                    ph->p_memsz - ph->p_filesz);
  }

  if (vma_start)
    *vma_start = _vma_start;
  if (vma_end)
    *vma_end = _vma_end;

  return eh->e_entry;
}
