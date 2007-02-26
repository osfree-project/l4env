/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/cxx/alloc.h>
#include <l4/cxx/iostream.h>
namespace L4 {

  void Alloc_list::free( void *blk, unsigned long size )
  {
    Elem *n = reinterpret_cast<Elem*>(blk);
    Elem **c = &_free;
    while (*c) 
      {
	if (reinterpret_cast<char*>(*c) + (*c)->size == blk)
	  {
	    (*c)->size += size;
	    blk = 0;
	    break;
	  }
	  
	if (reinterpret_cast<char*>(*c) > blk)
	  break;
	
	c = &((*c)->next);
      }

    if (blk)
      {
        n->next = *c;
	n->size = size;
	*c = n;
      }

    while (*c && reinterpret_cast<char*>(*c)+(*c)->size == (char*)((*c)->next))
      {
	(*c)->size += (*c)->next->size;
	(*c)->next = (*c)->next->next;
      }
  }

  void *Alloc_list::alloc( unsigned long size )
  {
    if (!_free) 
      return 0;

    // best fit;
    Elem **min = 0;
    Elem **c = &_free;

    while(*c) 
      {
	if ((*c)->size >= size && (!min || (*min)->size > (*c)->size))
	  min = c;

	c = &((*c)->next);
      }

    if (!min)
      return 0;


    void *r;
    if ( (*min)->size > size )
      {
	r = reinterpret_cast<char*>(*min) + ((*min)->size - size);
	(*min)->size -= size;  
      }
    else
      {
	r = *min;
	*min = (*min)->next;
      }

    return r;
  }

}

