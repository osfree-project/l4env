INTERFACE [smas]:

#include "cpu_lock.h"
#include "lock_guard.h"
#include "mem_layout.h"
#include "paging.h"
#include "types.h"

class Space;

/** Small address space managment class.
 */
class Smas
{
  friend class Jdb_kern_info_smas;

private:
  /// Number of small address spaces
  int _space_count;

  /// Size of a single space in 4MB blocks.
  int _space_size;

  /// Overall available size in 4MB blocks, just for convenience.
  static const unsigned _available_size =
    ((Mem_layout::Smas_end - Mem_layout::Smas_start) >> Pd_entry::Shift)
        & Pd_entry::Mask;

  /** Table with references to the occuping spaces for all 4MB pages.
   * This table restricts the space usable for small spaces to 512 MB. 
   * Make it bigger if needed.
   */
  Space* _spaces[_available_size];
};

extern Smas smas;

IMPLEMENTATION [smas]:

#include <cstdio>
#include <cassert>

#include "config.h"
#include "context.h"
#include "cpu_lock.h"
#include "globals.h"
#include "lock_guard.h"
#include "kmem.h"
#include "mem_layout.h"
#include "mem_unit.h"
#include "paging.h"
#include "panic.h"
#include "space.h"
#include "space.h"
#include "std_macros.h"
#include "vmem_alloc.h"


extern "C" char tramp_small_after_sysexit_func;
extern "C" char tramp_small_end;

PUBLIC
Smas::Smas() 
{
  _space_count = -1;
  memset (_spaces, 0, _available_size*sizeof(_spaces[0]));

  if (! Vmem_alloc::page_alloc ((void*) Mem_layout::Smas_trampoline,
				Vmem_alloc::NO_ZERO_FILL, Page::USER_NO))
    panic("Cannot allocate trampoline page for smas");

  // copy the code (position independant) to the trampoline page
  memcpy((void*)Mem_layout::Smas_trampoline, 
         (const void*)&tramp_small_after_sysexit_func, 
         &tramp_small_end - &tramp_small_after_sysexit_func);

  // make page readable by user
  Vmem_alloc::page_attr ((void*) Mem_layout::Smas_trampoline, Page::USER_RO);
}

/** 
 * Translate a linear address to one in a small space.
 * Call in locked mode if you want to make sure that the values
 * are still up to date on return.
 *
 * @param linearaddr The linar address.
 * @param space On return contains the space, that resides at this address.
 * @param smalladdr On return contains the virtual address within 
 *                  the address space of the space in question.
 * @return true, if the address belongs to a small space and it is
 *         occupied.
 */
PUBLIC inline NEEDS ["config.h", "mem_layout.h"]
bool
Smas::linear_to_small(Address linearaddr, Space** space,
                      Address* smalladdr)
{
  // bounds checking
  if (linearaddr <  Mem_layout::Smas_start || 
      linearaddr >= Mem_layout::Smas_end   ||
      _space_count <= 0)  // have small spaces at all?
    return false;

  *space = _spaces[(linearaddr - Mem_layout::Smas_start) 
		    >> Config::SUPERPAGE_SHIFT];

  if (*space == 0)
    return false;

  *smalladdr = (linearaddr - Mem_layout::Smas_start) 
		    % (_space_size * Config::SUPERPAGE_SIZE);
  return true;
}


/** 
 * Lookup space in table.
 */ 
PRIVATE inline NEEDS ["mem_layout.h"]
int
Smas::lookup(Space *space) const
{
  //calculate expected position from spaces data segment
  unsigned sbase = space->small_space_base();

  if (sbase < Mem_layout::Smas_start)
    return -1;

  unsigned index = Pdir::virt_to_idx(sbase - Mem_layout::Smas_start);

  //now check if it is where expected in the table 
  assert(index < _available_size && _spaces[index] == space);

  return index;
}


/** 
 * Move space around.
 * @param index If 0 the space will be moved to the big space, otherwise
 *              to the given small spaces. If it doesn't exist or is
 *              occupied nothing happens.
 */
PUBLIC
void
Smas::move(Space *space, int index)
{
  // silently ignore requests out of bounds
  if (index > _space_count) 
    return;

  Lock_guard<Cpu_lock> guard (&cpu_lock);

  int old_space = lookup(space);
  int new_space = index == 0 ? -1 : (index - 1) * _space_size;

  // already there?
  if (old_space == new_space)
    return;

  // already occupied?
  if (new_space >= 0)
    {
      for (int i = new_space; i < new_space + _space_size; i++)
	if (_spaces[i] != 0) 
	  return;
    }

  // move out
  if (old_space >= 0) 
    {
      Address base;
      int i;
      for (i = old_space, base = space->small_space_base();
	   i < old_space + _space_size; 
	   i++, base += Config::SUPERPAGE_SIZE)
	{
	  _spaces[i] = 0;
	  Kmem::update_smas_window(base, 0, true);
	}
      Mem_unit::tlb_flush();
    }

  // move in entry
  if (index > 0)
    {
      Address baseaddr = Mem_layout::Smas_start 
		       + (new_space << Config::SUPERPAGE_SHIFT);
      space->set_small_space( baseaddr, 
			     _space_size << Config::SUPERPAGE_SHIFT );
      
      for (int i = new_space; i < new_space + _space_size; i++) 
	_spaces[i] = space;
    } 
  else 
    {
      space->set_small_space(0, Kmem::mem_user_max);
      //If we try to change the current, reset the context.
      if (current()->space() == space)
	space->switchin_context();
    }

  barrier();
}


/** 
 * Sets size of the small address spaces.
 * If the size is different from the current size, all tasks
 * that are still in small spaces will be moved to the big one.
 * @param size Size of a address space in Config::SUPERPAGE_SIZEs. 
 *             For Pentium this is the number of 4 MB blocks.
 */
PUBLIC
void 
Smas::set_space_size(size_t size)
{ 
  // nothing to change or size to big
  if (size == unsigned(_space_size) || size > _available_size)
    return;

  Lock_guard <Cpu_lock> guard (&cpu_lock);

  // throw out all the spaces that still reside up there
  Space *last_space = 0;
  for (unsigned i = 0; i < _available_size; i++) 
    {
      if (_spaces[i]) 
	{
	  if (_spaces[i] != last_space) 
	    {
	      _spaces[i]->set_small_space(0, Kmem::mem_user_max);
	      // if we try to change the current, reset the context
	      if (current()->space() == _spaces[i])
      		_spaces[i]->switchin_context();
	      last_space = _spaces[i];
	    }
	  _spaces[i] = 0;
	}
    }

  // now new size
  _space_size  = size;
  _space_count = _available_size / _space_size;
  if (_space_count > 127) 
    _space_count = 127;
}


/** 
 * Returns size and number a space is in.
 * This function does not give reliable information on the assignment of
 * small address spaces as a space might be thrown out of a small space at any
 * time due to a segmentation fault or movements initiated via a system call.
 *
 * @return same format as value returned by the thread_schedule system call.
 */
PUBLIC
int
Smas::space_nr(Space* space) 
{
  unsigned sbase = space->small_space_base();
  unsigned spage;

  if (_space_count < 0 || sbase < Mem_layout::Smas_start)
    return 0;

  spage = (sbase - Mem_layout::Smas_start) / Config::SUPERPAGE_SIZE;
  return 2*spage + 3*_space_size;
}

/**
 * The single instance of Smas.
 */
Smas smas;

