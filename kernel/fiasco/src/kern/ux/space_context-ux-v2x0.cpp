INTERFACE:

EXTENSION class Space_context
{
public:
  Space_context (unsigned taskno);
};

IMPLEMENTATION[ux-v2x0]:

#include "hostproc.h"

IMPLEMENT
Space_context::Space_context (unsigned taskno)
{
  memset(_dir, 0, Config::PAGE_SIZE);

  _dir[pid_index] = Hostproc::create (taskno) << 8;
}

