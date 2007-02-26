INTERFACE:

#include "config.h"
#include "l4_types.h"
#include "initcalls.h"

class Name_entry
{
public:
  enum
  {
    Size         = 19,
  };

private:
  Global_id      _id;
  char           _name[Size];
};

class Jdb_thread_names
{
private:
  enum
  {
    Name_pages   = 2,
    Name_entries = (Name_pages<<Config::PAGE_SHIFT) / sizeof(Name_entry),
  };

  static Name_entry *_names;
};

IMPLEMENTATION [arm-debug]:

#include "entry_frame.h"
#include "thread.h"
#include "static_init.h"


static void register_extension(Thread *, Entry_frame *r)
{
  Jdb_thread_names::register_thread(r->r[0], (char*)(r->r[1]));
}

static void init_dbg_extension()
{
  Thread::dbg_extension[0x10] = &register_extension;
}

STATIC_INITIALIZER(init_dbg_extension);

IMPLEMENTATION:

#include <cstdio>

#include <feature.h>
#include "kmem_alloc.h"
#include "panic.h"
#include "space.h"
#include "static_init.h"

Name_entry *Jdb_thread_names::_names;

PUBLIC inline NOEXPORT
bool
Name_entry::is_invalid() const
{ return _id.is_invalid(); }

PUBLIC inline NOEXPORT
void
Name_entry::invalidate()
{ _id = Global_id::Invalid; }

PUBLIC inline NOEXPORT
Global_id
Name_entry::id() const
{ return _id; }

PUBLIC inline
const char *
Name_entry::name() const
{ return this ? _name : ""; }

PUBLIC
void
Name_entry::set(Global_id id, const char *name)
{
  unsigned i;

  _id = id;
  for (i=0; i<sizeof(_name)-1; i++)
    {
      _name[i] = current_mem_space()->peek_user(name++);
      if (!_name[i])
	break;
    }
  _name[i] = 0;
}

PUBLIC static FIASCO_INIT
void
Jdb_thread_names::init()
{
  _names = (Name_entry*)Kmem_alloc::allocator()->unaligned_alloc(Name_pages);
  if (!_names)
    panic("No memory for thread names");

  for (int i=0; i<Name_entries; i++)
    _names[i].invalidate();
}

PUBLIC static
void
Jdb_thread_names::register_thread(Global_id id, const char *name)
{
  if (!name)
    {
      Name_entry *n;

      if ((n = lookup(id, false)))
	n->invalidate();
      return;
    }

  Global_id id1, id2;
  id1 = id;
  id1.version(0);
  for (int i=0; i<Name_entries; i++)
    {
      id2 = _names[i].id();
      id2.version(0);
      if (id1 == id2 || _names[i].is_invalid())
	{
	  _names[i].set(id, name);
	  return;
	}
    }
}

PUBLIC static
Name_entry *
Jdb_thread_names::lookup(Global_id id, bool strict)
{
  for (int i=0; i<Name_entries; i++)
    if (( strict && _names[i].id() == id) ||
	(!strict && _names[i].id().d_task()   == id.d_task()
	         && _names[i].id().d_thread() == id.d_thread()))
      return &_names[i];

  return 0;
}

STATIC_INITIALIZE(Jdb_thread_names);

KIP_KERNEL_FEATURE("thread_names");
