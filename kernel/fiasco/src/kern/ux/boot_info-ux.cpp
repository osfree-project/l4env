/*
 * Fiasco-UX
 * Architecture specific bootinfo code
 */

INTERFACE:

#include <flux/x86/multiboot.h>
#include <sys/types.h>			// for pid_t

EXTENSION class Boot_info 
{
public:
  static Address			mbi_phys();
  static multiboot_info const *		mbi_virt();
  static int				fd();
  static pid_t				pid();
  static char **			args();
  static bool				debug_ctxt_switch();
  static bool				debug_page_faults();

private:
  static int				phys_fd;	// Physmem FD
  static pid_t				proc_id;	// Process ID
  static char **			cmd_args;	// Cmd Line Parameters
  static struct multiboot_info *	mbi;		// Multiboot Info
  static bool				_debug_ctxt_switch;
  static bool				_debug_page_faults;
};

IMPLEMENTATION[ux]:

#include <cassert>			// for assert
#include <cerrno>			// for errno
#include <cstdlib>			// for atol
#include <cstring>			// for stpcpy
#include <cstdio>			// for printf
#include <fcntl.h>			// for open
#include <panic.h>			// for panic
#include <unistd.h>			// for getopt
#include <sys/mman.h>			// for mmap
#include <sys/stat.h>			// for open

#include "initcalls.h"
#include "linker_syms.h"
#include "loader.h"

int			Boot_info::phys_fd;
pid_t			Boot_info::proc_id;
char **			Boot_info::cmd_args;
struct multiboot_info *	Boot_info::mbi;

bool Boot_info::_debug_ctxt_switch;
bool Boot_info::_debug_page_faults;

IMPLEMENT FIASCO_INIT
void
Boot_info::init (void)
{
  extern int			__libc_argc;
  extern char **		__libc_argv;
  const char *			modules[16] = { "sigma0-ux", "rmgr-ux", };
  const char *			physmem_file = "/tmp/physmem";
  struct multiboot_module *	mbm;
  int				i, arg, modcount = 2;
  char *			modname, *ptr;
  unsigned long int		memsize = 32 << 20;

  cmd_args = __libc_argv;
  proc_id  = getpid ();

  /* First determine the desired memory size */
  while ((arg = getopt (__libc_argc, __libc_argv, "cf:l:m:p?R:S:")) != -1) {
    switch (arg) {
      case 'S':
	modules[0] = optarg;
	break;
      case 'R':
	modules[1] = optarg;
	break;
      case 'c':
        _debug_ctxt_switch = true;
        break;
      case 'f':
        physmem_file = optarg;
        break;
      case 'l':
        if (modcount < 16)
          modules[modcount++] = optarg;
        break;
      case 'm':
        memsize = atol (optarg) << 20;
        break;
      case 'p':
        _debug_page_faults = true;
        break;
      case '?':
        printf ("%s\n\n"
		"-f file    : Specify the location of the physical memory backing store\n"
                "-l module  : Specify which module(s) to load (not for sigma0 and rmgr)\n"
                "-m memsize : Specify physical memory size in MB (currently up to 1024)\n"
                "-c         : Display all context switches\n"
                "-p         : Display all page faults\n"
		"-R         : Specify non-standard rmgr (maybe for X0)\n"
		"-S         : Specify non-standard sigma0 (maybe for X0 again)\n",
                __libc_argv[0]);
        exit (0);
    }
  }

  puts ("\n\nFiasco UX Startup...");

  if ((phys_fd = open (physmem_file, O_RDWR | O_CREAT | O_EXCL | O_TRUNC, 0700)) == -1)
    panic ("cannot create physmem: %s", strerror (errno));

  unlink (physmem_file);

  // Cannot handle more physical memory than the space we have for mapping it.
  if (memsize > reinterpret_cast<unsigned int>(&_linuxstack) - reinterpret_cast<unsigned int>(&_physmem_1))
    memsize = reinterpret_cast<unsigned int>(&_linuxstack) - reinterpret_cast<unsigned int>(&_physmem_1);

  // Cannot handle more physical memory than rmgr can handle (currently 1 GB)
  // rmgr considers anything greater than 0x40000000 as adapter page.
  if (memsize > 1 << 30)
    memsize = 1 << 30;

  if (ftruncate (phys_fd, memsize) == -1)
    panic ("cannot resize physmem: %s", strerror (errno));

  if (mmap (reinterpret_cast<void *>(&_physmem_1), memsize, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, phys_fd, 0) == MAP_FAILED)
    panic ("cannot mmap physmem: %s", strerror (errno));

  printf ("Mapping %lu MB Physmem on FD %d\n", memsize >> 20, phys_fd);

  mbi = reinterpret_cast<struct multiboot_info *>(&_physmem_1);    
  mbm = reinterpret_cast<struct multiboot_module *>((char *) mbi + sizeof (*mbi));

  // Load the modules
  for (i = 0; i < modcount; i++) {

    ptr = strchr (modules[i], ' ');

    if (ptr)		// Cut off between module name and command line
      *ptr = 0;

    if (Loader::load_elf_image (memsize, modules[i], mbm + i) == -1) {
      printf ("Cannot load module: %s\n", modules[i]);
      abort ();
    }

    if (ptr)		// Reattach command line
      *ptr = ' ';
  }

  // Setup module names; all addresses physical
  for (i = 0, modname = (char *)(mbm + modcount); i < modcount; i++) {
    mbm[i].string = (vm_offset_t) ((char *) modname - (char *) mbi);
    ptr = strrchr (modules[i], '/');		// Strip path
    modname = stpcpy (modname, ptr ? ptr + 1 : modules[i]) + 1;
  }

  mbi->flags      = MULTIBOOT_MEMORY;
  mbi->mem_lower  = 0;               
  mbi->mem_upper  = memsize >> 10;            /* in KB */
  mbi->mods_count = modcount;
  mbi->mods_addr  = (0 + sizeof (*mbi));

  puts ("\nBootstrapping...\n");
}

IMPLEMENT inline
Address
Boot_info::mbi_phys()
{
  return 0;
}

IMPLEMENT inline
multiboot_info const *
Boot_info::mbi_virt()
{
  return mbi;
}

IMPLEMENT inline
int
Boot_info::fd()
{
  return phys_fd;
}

IMPLEMENT inline
pid_t
Boot_info::pid()
{
  return proc_id;
}

IMPLEMENT inline
char **
Boot_info::args()
{
  return cmd_args;
}

IMPLEMENT inline
bool
Boot_info::debug_ctxt_switch()
{
  return _debug_ctxt_switch;
}

IMPLEMENT inline
bool
Boot_info::debug_page_faults()
{
  return _debug_page_faults;
}
