INTERFACE:

#include "space_index.h"
#include "mem_space.h"

class Ram_quota;
class Io_space;

class Space
{
  friend class Jdb_kern_info_smas;

private:
  // DATA
  Mem_space _mem_space;
  Task_num _id;
  Task_num _chief;
};

IMPLEMENTATION:

#include "config.h"
#include "l4_types.h"

// 
// class Space
// 

/** Constructor.  Creates a new address space and registers it with
  * Space_index.
  *
  * Registration may fail (if a task with the given number already
  * exists, or if another thread creates an address space for the same
  * task number concurrently).  In this case, the newly-created
  * address space should be deleted again.
  * 
  * @param number Task number of the new address space
  */
PUBLIC
Space::Space (Ram_quota *q, unsigned number)
  : _mem_space(q),
    _id (number),
    _chief (~0UL)
#ifdef CONFIG_TASK_CAPS
    ,_cap_space(q)
#endif
{
  Task_num chief;

  // register in task table
  if (! Space_index::add (this, number, &chief))
    return;

  _chief = chief;

  init_task_caps();
  init_io_space();
}

PROTECTED
Space::Space (unsigned number, unsigned chief, Mem_space::Dir_type* pdir)
  : _mem_space (pdir),
    _id (number),
    _chief (chief)
#ifdef CONFIG_TASK_CAPS
    ,_cap_space(Ram_quota::root)
#endif
{
  init_task_caps();
  init_io_space();
}

PUBLIC
Space::~Space ()
{
  if (_chief != ~0U)
    {
      // deregister from task table
      remove_from_space_index();
    }
}

PUBLIC inline
Ram_quota *
Space::ram_quota() const
{ return _mem_space.ram_quota(); }

PROTECTED
void
Space::reset_dirty ()
{
  _mem_space.reset_dirty ();
  _chief = ~0U;
}

PRIVATE inline
void
Space::remove_from_space_index()
{
  Space_index::del (id(), chief());
}

/**
 * Task number.
 * @return Number of the task to which this Space instance belongs.
 */
PUBLIC inline 
Space_index
Space::id() const
{
  return Space_index(_id);
}

/**
 * Chief Number.
 * @return Task number of this task's chief.
 */
PUBLIC inline 
Space_index 
Space::chief() const
{
  return Space_index(_chief);
}

PUBLIC inline 
void 
Space::switchin_context ()
{
  _mem_space.switchin_context();
}

/// Lookup space belonging to a task number.
PUBLIC static inline
Space *
Space::id_lookup (Task_num id)
{
  return Space_index (id).lookup();
}

// Mem_space utilities

/// Return memory space.
PUBLIC inline
Mem_space const * 
Space::mem_space () const
{ return &_mem_space; }

PUBLIC inline
Mem_space* 
Space::mem_space ()
{
  return &_mem_space;
}

PUBLIC static inline NEEDS[Space::id_lookup]
bool
Space::lookup_space (Task_num id, Mem_space** out_space)
{
  Space* s = id_lookup (id);
  if (s) 
    *out_space = s->mem_space();

  return s;
}

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION[!io]:

/// Is this task a privileged one?
PUBLIC static inline 
bool 
Space::is_privileged () 
{
  return true;
}

PRIVATE static inline
void
Space::init_io_space ()
{}

