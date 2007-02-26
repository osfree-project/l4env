INTERFACE:

#include "space_context.h"
#include "config.h"

class Smas;

/** Small address space managment class.
 */
class Smas
{
  private:
    /// Number of small address spaces
    int _space_count;
    /// Size of a single space in 4MB blocks.
    int _space_size;
    /// Overall available size in 4MB blocks. Just for convenience.
    unsigned _available_size; 
    /** Table with references to the occuping spaces
      *	for all 4MB pages.
      * This table restricts the space usable for small spaces
      * to 512 MB. Make it bigger if needed.	
      */
    Space_context* spaces[128];
};

extern Smas smas;

IMPLEMENTATION:

#include <cstdio>
#include <cassert>

#include "context.h"
#include "cpu_lock.h"
#include "globals.h"
#include "kdb_ke.h"
#include "kmem.h"
#include "space.h"
#include "panic.h"



/** Constructor.
 */
PUBLIC
Smas::Smas () {
  _space_count = -1;
  _available_size = ((Kmem::smas_end - Kmem::smas_start) 
	                                     >> PDESHIFT) & PDEMASK;
  if (_available_size > 128 )
	  panic("Too much space for small address spaces.");

  for (int i=0; i < 128; i++ ) {
	 spaces[i] = 0;
  }
}

/** Translate a linear address to one in a small space.
 *  Call in locked mode if you want to make sure that the values
 *  are still up to date on return.
 *
 * @param linearaddr The linar address.
 * @param space On return contains the space, that resides at this address.
 * @param smalladdr On return contains the virtual address within 
 *                  the address space of the space in question.
 * @return true, if the address belongs to a small space and it is
 *         occupied.
 */
PUBLIC inline
bool Smas::linear_to_small( vm_offset_t linearaddr,
                             Space_context** space,
                             vm_offset_t* smalladdr )
{
  // bounds checking
  if ( linearaddr < Kmem::smas_start || linearaddr >= Kmem::smas_end
       || _space_count <= 0)  //have small spaces at all?
  {
    return false;
  }

  *space = spaces[(linearaddr - Kmem::smas_start) >> Config::SUPERPAGE_SHIFT];

  if (*space == 0 ) {
    return false;
  }

  *smalladdr = (linearaddr - Kmem::smas_start) % ( _space_size * Config::SUPERPAGE_SIZE);


  return true;

}


/** Lookup space in table.
 * 
 */ 
PRIVATE inline
int Smas::lookup( Space_context* space ) const
{
  //calculate expected position from spaces data segment
  unsigned sbase = space->small_space_base();

  if  (sbase < Kmem::smas_start ) return (-1);
  
  unsigned index = ((sbase - Kmem::smas_start) >> PDESHIFT) & PDEMASK;

  //now check if it is where expected in the table 
  assert(index < _available_size & spaces[index] == space);

  return index;
}


/** Move space around.
 *  @param index    If 0 the space will be moved to the big space, otherwise
 *                  to the given small spaces. If it doesn't exist or is
 *                  occupied nothing happens.
 */
PUBLIC
void Smas::move( Space_context* space, int index )
{
  //silently ignore requests out of bounds
  if ( index > _space_count ) return;

  bool was_locked = cpu_lock.test_and_set();

  int old_space = lookup( space );
  int new_space = (index == 0)?(-1):((index - 1) * _space_size);

  //already there?
  if ( old_space == new_space ) {
    cpu_lock.set( was_locked );
    return;
  }
  
  // already occupied?
  if (new_space >= 0 ) {
    for (int i = new_space; i < new_space + _space_size; i++) {
  	    if (spaces[i] != 0) {
			 cpu_lock.set( was_locked );
			 return;
		 }
    }
  }

  // start to actually do something: 

  //Move out:
  if ( old_space >= 0 ) {
    vm_offset_t base;
    int i;
    for (i = old_space, base = space->small_space_base(); 
	   i < old_space + _space_size; 
	   i++, base += Config::SUPERPAGE_SIZE ) {
      spaces[i] = 0;
      Kmem::update_smas_window( base, 0, true );
    }
    Kmem::tlb_flush();
  }
  
  //move in entry
  if ( index > 0 ) {
    vm_offset_t baseaddr = Kmem::smas_start + (new_space << Config::SUPERPAGE_SHIFT);
    space->set_small_space( baseaddr,  _space_size << Config::SUPERPAGE_SHIFT );
    for (int i = new_space; i < new_space + _space_size; i++) {
      spaces[i] = space;
	 }
  } else {
    space->set_small_space( 0, Kmem::mem_user_max );
    //If we try to change the current, reset the context.
    if ( current()->space_context() == space ) {
      space->switchin_context();
    }
  }
  
 cpu_lock.set( was_locked );

#if 0
 printf ("KERN: Space movement from %u to %u successful.\n",
            (old_space == -1)?0:(old_space / _space_size) + 1, index );
#endif
}


/** Sets size of the small address spaces.
 *  If the size is different from the current size, all tasks
 *  that are still in small spaces will be moved to the big one.
   @param size Size of a address space in Config::SUPERPAGE_SIZEs. 
 *              For Pentium this is the number of 4 MB blocks.
 */
PUBLIC void 
Smas::set_space_size( size_t size )
{ 
  //nothing to change or size to big
  if ( size == unsigned(_space_size) || size > _available_size )
    return;
 
  
  bool was_locked = cpu_lock.test_and_set();
  
  //throw out all the spaces that still reside up there
  Space_context *last_space = 0;
  for (unsigned i = 1; i <= _available_size; i++) {
    if (spaces[i]) {
      if (spaces[i] != last_space) {
        spaces[i]->set_small_space( 0, Kmem::mem_user_max );
        //if we try to change the current, reset the context
        if ( current()->space_context() == spaces[i] ) {
          spaces[i]->switchin_context();
        }
        last_space = spaces[i];
      }
      spaces[i] = 0;
    }
  }
    
  //now new size
  _space_size = size;
  _space_count = _available_size / _space_size;
  if (_space_count > 127) _space_count = 127;

  cpu_lock.set( was_locked );

#if 0  
  printf("KERN: New Segment layout: number: %u, size: 0x%x.\n",
             _space_count,  _space_size * Config::SUPERPAGE_SIZE );
#endif
}


/** Returns size and number a space is in.
 * This function does not give reliable information on the assignment of
 * small address spaces as a space might be thrown out of a small space at any
 * time due to a segmentation fault or movements initiated via a system call.
 *
 *  @return same format as value returned by the thread_schedule system call.
 */
PUBLIC
int Smas::space_nr( Space_context* space) {

  unsigned sbase = space->small_space_base();

  if (_space_count < 0 || sbase < Kmem::smas_start ) 
    return 0;

  /* This is the original formula (see L4 spec):
     int index = (((sbase - Kmem::smas_start) 
                      / ( _space_size * Config::SUPERPAGE_SIZE) ) + 1;
     return ((2 * index) * _space_size + _space_size;

     which can be canceled to:
      (even less readable but at least fast and somehow fascinating)
  */
 
  unsigned spage = (sbase - Kmem::smas_start) / Config::SUPERPAGE_SIZE;

  return ( 2 * spage) + 3 * _space_size;   

  /* in order to make also a segment of the full taskwindow
   * size working, we do not deliver a value here
   */
  return 0xFF;
}

/**
 *  The single instance of Smas: smas
 */
Smas smas;



