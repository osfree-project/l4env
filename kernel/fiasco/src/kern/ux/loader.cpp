
INTERFACE:

#include <flux/x86/multiboot.h>

class Loader
{
private:
  static unsigned int phys_base;

public:
  static int load_elf_image	(unsigned long int memsize,
				 const char * const path,
				 struct multiboot_module *module);
};

IMPLEMENTATION:

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <elf.h>
#include "initcalls.h"
#include "linker_syms.h"

unsigned int Loader::phys_base = reinterpret_cast<vm_offset_t>(&_physmem_1);

IMPLEMENT FIASCO_INIT
int
Loader::load_elf_image (unsigned long int memsize, const char * const path,
                        struct multiboot_module *module) {

  FILE *fp;
  Elf32_Ehdr eh;
  Elf32_Phdr ph;
  long offset;
  int i;
  
  if ((fp = fopen (path, "r")) == NULL)
    return -1;

  // Load ELF Header
  if (fread (&eh, sizeof (eh), 1, fp) != 1)
    goto error;

  // Check if valid ELF magic, 32bit, little endian, SysV
  if (*(unsigned *) eh.e_ident != *(unsigned *) ELFMAG ||
      eh.e_ident[EI_CLASS] != ELFCLASS32  ||
      eh.e_ident[EI_DATA]  != ELFDATA2LSB ||
      eh.e_ident[EI_OSABI] != ELFOSABI_SYSV)
    goto error;

  // Check if executable, i386 format, current ELF version
  if (eh.e_type    != ET_EXEC ||
      eh.e_machine != EM_386  ||
      eh.e_version != EV_CURRENT)
    goto error;

  printf ("\nLoading Module 0x%08x [%s]\n", module->reserved = eh.e_entry, path);

  // Load all program sections
  for (i = 0, offset = eh.e_phoff; i < eh.e_phnum; i++) {

    // Load Program Header
    if (fseek (fp, offset, SEEK_SET) == -1 ||
        fread (&ph, sizeof (ph), 1, fp) != 1)
      goto error;

    offset += sizeof (ph);

    if (ph.p_type != PT_LOAD)
      continue;

    // Check if section fits into memory
    if (ph.p_vaddr + ph.p_memsz > memsize)
      goto error;

    // Load Section
    if (fseek (fp, ph.p_offset, SEEK_SET) == -1 ||
        fread ((void *)(phys_base + ph.p_vaddr), ph.p_filesz, 1, fp) != 1)
      goto error;

    // Zero-pad uninitialized data if filesize < memsize
    if (ph.p_filesz < ph.p_memsz)
      memset ((void *)(phys_base + ph.p_vaddr + ph.p_filesz), 0,
               ph.p_memsz - ph.p_filesz);

    printf ("Loading Region 0x%08x-0x%08x [%s%s%s]\n",
            ph.p_vaddr, ph.p_vaddr + ph.p_memsz,
            "r" + !(ph.p_flags & PF_R),
            "w" + !(ph.p_flags & PF_W),
            "x" + !(ph.p_flags & PF_X));

    if (i == 0)
      module->mod_start = ph.p_vaddr;
    if (i == eh.e_phnum - 1)
      module->mod_end   = ph.p_vaddr + ph.p_memsz;
  }

  fclose (fp);
  return 0;

error:
  fclose (fp);
  return -1;
}
