INTERFACE [abs_syscalls,rel_syscalls]:

#include "initcalls.h"

class Sys_call_page
{
public:

  static void init() FIASCO_INIT;
};

IMPLEMENTATION [abs_syscalls,rel_syscalls]:

#include <feature.h>
KIP_KERNEL_FEATURE("kip_syscalls");
