INTERFACE:

EXTENSION class Space_context
{
public:
  Space_context();
};

IMPLEMENTATION[ux-v4]:

#include "hostproc.h"

IMPLEMENT
Space_context::Space_context ()
{
  memset(_dir, 0, Config::PAGE_SIZE);

  _dir[pid_index] = Hostproc::create ((unsigned) this) << 8;
}

