INTERFACE:

EXTENSION class Kernel_thread
{
public:
  static int	init_done();

private:
  static int	free_initcall_section_done;
};

IMPLEMENTATION[ux]:

#include <unistd.h>
#include <sys/mman.h>
#include "boot_info.h"
#include "fb.h"
#include "kdb_ke.h"
#include "linker_syms.h"

int Kernel_thread::free_initcall_section_done;

IMPLEMENT inline
int
Kernel_thread::init_done()
{
  return free_initcall_section_done;
}

IMPLEMENT inline NEEDS [<unistd.h>, <sys/mman.h>, "linker_syms.h"]
void
Kernel_thread::free_initcall_section()
{
  munmap (&_initcall_start, &_initcall_end - &_initcall_start);
  free_initcall_section_done = 1;
}

IMPLEMENT inline NEEDS ["boot_info.h", "fb.h", "kdb_ke.h"]
void
Kernel_thread::bootstrap_arch()
{
  // install slow trap handler
  nested_trap_handler = base_trap_handler;
  base_trap_handler = thread_handle_trap;

  if (Boot_info::wait())
    kdb_ke ("Wait");

  Fb::enable();
}
