/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_CXX_ALLOC_H__
#define L4_CXX_ALLOC_H__

namespace L4 {

  class Alloc_list
  {
  public:
    Alloc_list() : _free(0) {}
    Alloc_list( void *blk, unsigned long size ) : _free(0) 
    { free( blk, size ); }

    void free( void *blk, unsigned long size );
    void *alloc( unsigned long size );
    
  private:
    struct Elem 
    {
      Elem *next;
      unsigned long size;
    };

    Elem *_free;
  };
};

#endif // L4_CXX_ALLOC_H__

