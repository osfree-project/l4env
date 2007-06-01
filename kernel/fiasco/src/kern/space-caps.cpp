INTERFACE:

class Sys_task_new_frame;


INTERFACE[caps]:

#include "cap_space.h"
#include "obj_space.h"

EXTENSION class Space
{
  Cap_space _cap_space;
  Obj_space _obj_space;
  bool _task_caps;
};

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION[caps]:

PRIVATE inline
void Space::init_task_caps()
{ 
  _task_caps = false;
}

PUBLIC inline
void
Space::enable_task_caps ()
{
  _task_caps = true;
}

PUBLIC inline
bool
Space::task_caps_enabled () const
{
  return _task_caps;
}

// 
// Utilities for map<Cap_space> and unmap<Cap_space>
// 
PUBLIC inline 
Cap_space*
Space::cap_space()
{
  return &_cap_space;
}

PUBLIC inline 
Obj_space*
Space::obj_space()
{
  return &_obj_space;
}

PUBLIC static inline NEEDS[Space::id_lookup]
bool
Space::lookup_space (Task_num id, Cap_space** out_cap_space)
{
  Space* s = id_lookup (id);
  if (s) 
    *out_cap_space = s->cap_space();

  return s;
}

PUBLIC static inline NEEDS[Space::id_lookup]
bool
Space::lookup_space (Task_num id, Obj_space** out_cap_space)
{
  Space* s = id_lookup (id);
  if (s) 
    *out_cap_space = s->obj_space();

  return s;
}

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION[!caps]:

// 
// Dummy implementations
// 
PRIVATE inline
void Space::init_task_caps()
{}

PUBLIC inline
void
Space::enable_task_caps (const Sys_task_new_frame * /*params*/)
{}

PUBLIC inline
bool
Space::task_caps_enabled () const
{
  return false;
}
