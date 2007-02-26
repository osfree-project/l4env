#include <cstdio>
#include <cstring>

#include "types.h"
#include "elf.h"
#include "load_elf.h"

//extern "C" char _binary_kernel_start;

Startup_func load_elf( const char *name, void *elf, Mword *vma_start, Mword *vma_end )
{
  char *_elf = (char *)elf;
  Elf32_Ehdr *eh = (Elf32_Ehdr *)(_elf);
  Elf32_Phdr *ph = (Elf32_Phdr *)(_elf + eh->e_phoff);
  Mword _vma_start = (Mword)-1;
  Mword _vma_end   = 0;

  putstr ("Loading ELF image (");
  putstr (name);
  printf (") at %p [entry %p]\n", _elf, (void *) eh->e_entry);

  for (int i = 0; i < eh->e_phnum; i++, ph++) {
    if (ph->p_type != PT_LOAD)
      continue;

    if(ph->p_vaddr < _vma_start) _vma_start = ph->p_vaddr;
    if((ph->p_vaddr + ph->p_memsz) > _vma_end ) _vma_end = ph->p_vaddr + ph->p_memsz;

    memcpy ((void *) ph->p_paddr, _elf + ph->p_offset, ph->p_filesz);

    if (ph->p_filesz < ph->p_memsz)
      memset ((void *)(ph->p_paddr + ph->p_filesz), 0, ph->p_memsz - ph->p_filesz);
  }

  if(vma_start) *vma_start = _vma_start;
  if(vma_end)   *vma_end   = _vma_end;
  return (Startup_func) eh->e_entry;
}
