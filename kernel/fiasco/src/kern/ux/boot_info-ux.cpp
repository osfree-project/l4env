/*
 * Fiasco-UX
 * Architecture specific bootinfo code
 */

INTERFACE:

#include <sys/types.h>			// for pid_t
#include <getopt.h>			// for struct option

EXTENSION class Boot_info 
{
public:
  static void				reset_checksum_ro();
  static int				fd();
  static pid_t				pid();
  static char **			args();
  static bool				is_native (unsigned task);
  static bool				irq0_disabled();
  static bool				wait();
  static unsigned long			input_size();
  static unsigned long			fb_size();
  static Address			fb_virt();
  static Address			fb_phys();
  static unsigned int			fb_width();
  static unsigned int			fb_height();
  static unsigned int			fb_depth();
  static const char *			fb_program();

private:
  static int				_fd;		// Physmem FD
  static pid_t				_pid;		// Process ID
  static char **			_args;		// Cmd Line Parameters
  static bool				_irq0_disabled;
  static bool				_wait;
  static unsigned long			_native;
  static unsigned long			_input_size;
  static unsigned long			_fb_size;
  static Address			_fb_virt;
  static Address			_fb_phys;
  static unsigned int			_fb_width;
  static unsigned int			_fb_height;
  static unsigned int			_fb_depth;
  static const char *			_fb_program;
  static struct option			_long_options[];
  static char				_help[];
  static char *				_modules[];
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

#include "emulation.h"
#include "initcalls.h"
#include "linker_syms.h"
#include "loader.h"

int			Boot_info::_fd;
pid_t			Boot_info::_pid;
char **			Boot_info::_args;
bool			Boot_info::_irq0_disabled;
bool			Boot_info::_wait;
unsigned long		Boot_info::_native;
unsigned long		Boot_info::_input_size;
unsigned long		Boot_info::_fb_size;
Address			Boot_info::_fb_virt = 0xc0000000;
Address			Boot_info::_fb_phys;
unsigned int		Boot_info::_fb_width;
unsigned int		Boot_info::_fb_height;
unsigned int		Boot_info::_fb_depth;
const char *		Boot_info::_fb_program = "ux_con";

// If you add options here, add them to getopt_long and help below
struct option Boot_info::_long_options[] FIASCO_INITDATA =
{
  { "data_module",		required_argument,	NULL, 'd' },
  { "physmem_file",		required_argument,	NULL, 'f' },
  { "help",			no_argument,		NULL, 'h' },
  { "load_module",		required_argument,	NULL, 'l' },
  { "memsize",			required_argument,	NULL, 'm' },
  { "native_task",		required_argument,	NULL, 'n' },
  { "wait",			no_argument,		NULL, 'w' },
  { "fb_program",		required_argument,	NULL, 'F' },
  { "fb_geometry",		required_argument,	NULL, 'G' },
  { "rmgr",			required_argument,	NULL, 'R' },
  { "sigma0",			required_argument,	NULL, 'S' },
  { "disable_irq0",		no_argument,		NULL, '0' },
  { 0, 0, 0, 0 }
};

// Keep this in sync with the options above
char Boot_info::_help[] FIASCO_INITDATA =
  "Usage: %s\n\n"
  "-f file    : Specify the location of the physical memory backing store\n"
  "-l module  : Specify an ELF module to load (not for sigma0 and rmgr)\n"
  "-d module  : Specify a DATA module to load\n"
  "-m memsize : Specify physical memory size in MB (currently up to 1024)\n"
  "-n number  : Allow the specified task number to perform native syscalls\n"
  "-w         : Enter kernel debugger on startup and wait\n"
  "-0         : Disable the timer interrupt generator\n"
  "-F         : Specify a different frame buffer program\n"
  "-G         : Geometry for frame buffer: widthxheight@depth\n"
  "-R         : Specify different rmgr binary\n"
  "-S         : Specify different sigma0 binary\n";

char *Boot_info::_modules[32] FIASCO_INITDATA =
{
  "sigma0-ux",
  "rmgr-ux",
   NULL,	// Symbols
   NULL		// Lines
};

IMPLEMENT FIASCO_INIT
void
Boot_info::init()
{
  extern int			__libc_argc;
  extern char **		__libc_argv;
  const char *			physmem_file = "/tmp/physmem";
  char				**m, *cmd, *ptr, *str, buffer[4096];
  int				arg;
  unsigned int			i, owner = 1, modcount = 4, skip = 2;
  unsigned long int		memsize = 64 << 20;
  struct multiboot_info *	mbi;
  struct multiboot_module *	mbm;

  _args = __libc_argv;
  _pid  = getpid ();

  *(str = buffer) = 0;

  // Parse command line
  while ((arg = getopt_long (__libc_argc, __libc_argv, "d:f:hl:m:n:wF:G:R:S:0",
                             _long_options, NULL)) != -1) {
    switch (arg) {

      case 'S':
        owner = 0;
	_modules[0] = optarg;
	break;

      case 'R':
        owner = 1;
	_modules[1] = optarg;
        if (strstr (optarg, " -symbols"))
          {
            _modules[2] = "Symbols";
            skip--;
          }
        if (strstr (optarg, " -lines"))
          {
            _modules[3] = "Lines";
            skip--;
          }
	break;

      case 'l':
        if (modcount < sizeof (_modules) / sizeof (*_modules))
          _modules[owner = modcount++] = optarg;
        break;

      case 'd':
        if (modcount < sizeof (_modules) / sizeof (*_modules))
          {
            // XXX: Insufficient bounds checking

            if (owner + 1 == modcount)
              {
                if ((cmd = strchr (_modules[owner], ' ')))
                  *cmd = 0;
                
                if ((ptr = strrchr (_modules[owner], '/')))
                  ptr++;
                else
                  ptr = _modules[owner];

                str += snprintf (str, sizeof (buffer) - (str - buffer),
                                 " task modname \"%s\"", ptr);
                if (cmd)
                  *cmd = ' ';
              }

            if ((cmd = strchr (optarg, ' ')))
              *cmd = 0;
                
            if ((ptr = strrchr (optarg, '/')))
              ptr++;
            else
              ptr = optarg;

            str += snprintf (str, sizeof (buffer) - (str - buffer),
                             " module modname \"%s\"", ptr);

            if (cmd)
              *cmd = ' ';

            _modules[modcount++] = optarg;
          }
        break;

      case 'f':
        physmem_file = optarg;
        break;

      case 'm':
        memsize = atol (optarg) << 20;
        break;

      case 'n':
        if ((i = strtoul (optarg, NULL, 10)) < sizeof (_native) * 8)
          _native |= 1 << i;
        break;

      case 'w':
        _wait = true;
        break;

      case '0':
	_irq0_disabled = true;
	break;

      case 'F':
	_fb_program = optarg;
	break;

      case 'G':
        if (sscanf (optarg, "%ux%u@%u", &_fb_width, &_fb_height, &_fb_depth) == 3)
          {
            _fb_size = _fb_width * _fb_height * (_fb_depth + 7 >> 3);
            _fb_size += ~Config::SUPERPAGE_MASK;
            _fb_size &=  Config::SUPERPAGE_MASK;	// Round up to 4 MB

            /*
             * Add another area for input memory. This is a superpage due to
             * KERNEL: XXX can't grant from 4M page. -> 4K would be enough
             */
            _input_size = Config::SUPERPAGE_SIZE;
          }
	break;

      default:
        printf (_help, *__libc_argv);
        exit (arg == 'h' ? EXIT_SUCCESS : EXIT_FAILURE);
    }
  }

  puts ("\n\nFiasco UX Startup...");

  if (_native)
    printf ("Native Syscall Map: 0x%lx\n", _native);

  if ((_fd = open (physmem_file, O_RDWR | O_CREAT | O_EXCL | O_TRUNC, 0700)) == -1)
    panic ("cannot create physmem: %s", strerror (errno));

  unlink (physmem_file);

  // Cannot handle more physical memory than rmgr can handle (currently 1 GB)
  // rmgr considers anything greater than 0x40000000 as adapter page.
  if (memsize > 1 << 30)
    memsize = 1 << 30;

  // The framebuffer starts beyond the physical memory
  _fb_phys = memsize;

  // Create a sparse file as backing store
  if (lseek (_fd, memsize + _fb_size + _input_size - 1, SEEK_SET) == -1 ||
      write (_fd, "~", 1) != 1)
    panic ("cannot resize physmem: %s", strerror (errno));

  // Now map the beast in our virtual address space
  if (mmap (reinterpret_cast<void *>(&_physmem_1),
            memsize + _fb_size + _input_size,
            PROT_READ | PROT_WRITE,
            MAP_FIXED | MAP_SHARED, _fd, 0) == MAP_FAILED)
    panic ("cannot mmap physmem: %s", strerror (errno));

  printf ("Mapped %lu MB Memory + %lu KB Framebuffer + %lu MB Input Area on FD %d\n\n", 
           memsize >> 20, _fb_size >> 10, _input_size >> 20, _fd);

  mbi             = mbi_virt();
  mbi->flags      = MULTIBOOT_MEMORY | MULTIBOOT_CMDLINE;
  mbi->mem_lower  = 0;               
  mbi->mem_upper  = memsize >> 10;            /* in KB */
  mbi->mods_count = modcount - skip;
  mbi->mods_addr  = mbi_phys() + sizeof (*mbi);
  mbm = reinterpret_cast<multiboot_module *>((char *) mbi + sizeof (*mbi));
  str = reinterpret_cast<char *>(mbm + modcount - skip);

  // Copying of modules starts at the top, right below the kmem reserved area
  Address load_addr = kmem_start (0xffffffff); 

  // But never beyond 256 MB because Sigma0 and Rmgr can't map that area
  if (load_addr > 256 << 20)
    load_addr = 256 << 20;

  // Load/copy the modules
  for (m = _modules; m < _modules + modcount; m++)
    {
      if (!*m)
        continue;

      // Cut off between module name and command line
      if ((cmd = strchr (*m, ' ')))
        *cmd = 0;

      // Load sigma0 and rmgr, just copy the rest
      arg = m < _modules + 2 ?
            Loader::load_module (*m, mbm, memsize) :
            Loader::copy_module (*m, mbm, &load_addr);

      if (arg == -1)
        {
          printf ("Cannot load module: %s\n", *m);
          exit (EXIT_FAILURE);
        }
  
      // Reattach command line
      if (cmd)
        *cmd = ' ';

      mbm->string = str - (char *) mbi + mbi_phys();

      // Copy module name with path and command line
      str = stpcpy (str, *m) + 1;

      // For rmgr, pass in the extra command line
      if (m == _modules + 1)
        str = stpcpy (--str, buffer) + 1;

      mbm++;
    }

  // If we have a framebuffer running, propagate it
  if (_fb_size)
    {
      struct vbe_controller *vbe = reinterpret_cast
            <vbe_controller *>(str);
      struct vbe_mode *vbi = reinterpret_cast
            <vbe_mode *>((char *) vbe + sizeof (*vbe));

      mbi->flags |= MULTIBOOT_VIDEO_INFO;
      mbi->vbe_control_info   = mbi_phys() + ((char *) vbe - (char *) mbi);
      mbi->vbe_mode_info      = mbi_phys() + ((char *) vbi - (char *) mbi);
      vbe->total_memory       = _fb_size >> 16;		/* 2^16 == 1024 * 64 */
      vbi->phys_base          = _fb_virt;
      vbi->y_resolution       = _fb_height;
      vbi->x_resolution       = _fb_width;
      vbi->bits_per_pixel     = _fb_depth;
      vbi->bytes_per_scanline = _fb_width * (_fb_depth + 7 >> 3);

      /* 
       * Setting up vbi->{red,green,blue}_mask_size and co
       * could be done here...
       * but right now it doesn't look as if we would need it
       */
    }

  puts ("\nBootstrapping...\n");
}

IMPLEMENT inline
void
Boot_info::reset_checksum_ro()
{}

IMPLEMENT inline NEEDS ["emulation.h"]
Address
Boot_info::mbi_phys()
{
  return Emulation::multiboot_frame;
}

IMPLEMENT inline NEEDS ["linker_syms.h"]
multiboot_info * const
Boot_info::mbi_virt()
{
  return reinterpret_cast<multiboot_info * const>(&_physmem_1 + mbi_phys());
}

IMPLEMENT inline
int
Boot_info::fd()
{
  return _fd;
}

IMPLEMENT inline
pid_t
Boot_info::pid()
{
  return _pid;
}

IMPLEMENT inline
char **
Boot_info::args()
{
  return _args;
}

IMPLEMENT inline
bool
Boot_info::is_native (unsigned task)
{
  return (_native & (1 << task)) != 0;
}

IMPLEMENT inline
bool
Boot_info::irq0_disabled()
{
  return _irq0_disabled;
}

IMPLEMENT inline
bool
Boot_info::wait()
{
  return _wait;
}

IMPLEMENT inline
unsigned long
Boot_info::input_size()
{
  return _input_size;
}

IMPLEMENT inline
unsigned long
Boot_info::fb_size()
{
  return _fb_size;
}

IMPLEMENT inline
Address
Boot_info::fb_virt()
{
  return _fb_virt;
}

IMPLEMENT inline
Address
Boot_info::fb_phys()
{
  return _fb_phys;
}

IMPLEMENT inline
unsigned int
Boot_info::fb_width()
{
  return _fb_width;
}

IMPLEMENT inline
unsigned int
Boot_info::fb_height()
{
  return _fb_height;
}

IMPLEMENT inline
unsigned int
Boot_info::fb_depth()
{
  return _fb_depth;
}

IMPLEMENT inline
const char *
Boot_info::fb_program()
{
  return _fb_program;
}
