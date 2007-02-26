
INTERFACE:

#include <cstdio>			// for FILE *
#include <flux/x86/multiboot.h>
#include "types.h"

class Loader
{
private:
  static unsigned int phys_base;

public:
  static FILE *open_module	(const char * const path);

  static int load_module	(const char * const path,
				 struct multiboot_module *module,
				 unsigned long int memsize);

  static int copy_module	(const char * const path,
				 struct multiboot_module *module,
				 Address *load_addr);

};

IMPLEMENTATION:

#include <cstdlib>
#include <cstring>
#include <elf.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "config.h"
#include "initcalls.h"
#include "linker_syms.h"

unsigned int Loader::phys_base = reinterpret_cast<Address>(&_physmem_1);

IMPLEMENT FIASCO_INIT
FILE *
Loader::open_module (const char * const path)
{
  char magic[3];		// 2 + 1 for the terminating \0
  FILE *fp;

  if ((fp = fopen (path, "r")) == NULL)
    return NULL;

  fgets (magic, sizeof (magic), fp);

  if (magic[0] == '\037' && magic[1] == '\213')		// GZIP
    {
      FILE *pp;
      char pipecmd[256];
      int c;

      fclose (fp);

      snprintf (pipecmd, sizeof (pipecmd), "zcat %s", path);

      if ((pp = popen (pipecmd, "r")) == NULL)
        return NULL;

      if ((fp = tmpfile()) == NULL)
        {
          pclose (pp);
          return NULL;
        }

      while ((c = fgetc (pp)) != EOF)
        fputc (c, fp);

      pclose (pp);
    }

  rewind (fp);

  return fp;
}

IMPLEMENT FIASCO_INIT
int
Loader::load_module (const char * const path,
                     struct multiboot_module *module,
                     unsigned long int memsize)
{
  FILE *fp;
  Elf32_Ehdr eh;
  Elf32_Phdr ph;
  long offset;
  int i;

  if ((fp = open_module (path)) == NULL)
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

  // Record entry point (initial EIP)
  module->reserved = eh.e_entry;

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

    if (i == 0)
      module->mod_start = ph.p_vaddr;
    if (i == eh.e_phnum - 1)
      module->mod_end   = ph.p_vaddr + ph.p_memsz;
  }

  printf ("Loading Module 0x%08x-0x%08x [%s]\n",
          module->mod_start, module->mod_end, path);

  fclose (fp);
  return 0;

error:
  fclose (fp);
  return -1;
}

IMPLEMENT FIASCO_INIT
int
Loader::copy_module (const char * const path,
		     struct multiboot_module *module,
                     Address *load_addr)
{
  FILE *fp;
  struct stat s;
  int ret = -1;
  
  if ((fp = open_module (path)) == NULL)
    return ret;

  if (fstat (fileno (fp), &s) == -1)
    goto error;

  *load_addr -= s.st_size;
  *load_addr &= Config::PAGE_MASK;	// this may not be necessary

  printf ("Copying Module 0x%08x-0x%08lx [%s]\n",
          *load_addr, *load_addr + s.st_size, path);

  if (fread ((void *)(phys_base + *load_addr), s.st_size, 1, fp) == 1)
    {
      module->mod_start = *load_addr;
      module->mod_end   = *load_addr + s.st_size;
      ret = 0;
    }

error:
  fclose (fp);
  return ret;
}
