IMPLEMENTATION[ux]:

#include <unistd.h>
#include <sys/mman.h>
#include "linker_syms.h"
#include "pic.h"

IMPLEMENT inline NEEDS [<unistd.h>, <sys/mman.h>, "linker_syms.h"]
void
Kernel_thread::free_initcall_section()
{
  munmap (&_initcall_start, &_initcall_end - &_initcall_start);
}

IMPLEMENT inline NEEDS ["pic.h"]
void
Kernel_thread::bootstrap_arch()
{
  // 
  // install our slow trap handler
  //
  nested_trap_handler = base_trap_handler;
  base_trap_handler = thread_handle_trap;

  Pic::enable(0);
}
