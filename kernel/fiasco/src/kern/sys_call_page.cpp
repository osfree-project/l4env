INTERFACE:

#include "initcalls.h"

class Sys_call_page
{
public:

  static void init() FIASCO_INIT;

};


IMPLEMENTATION:

#include "static_init.h"

STATIC_INITIALIZE(Sys_call_page);
