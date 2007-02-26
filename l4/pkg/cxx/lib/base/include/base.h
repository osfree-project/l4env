/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_CXX_BASE_H__
#define L4_CXX_BASE_H__

#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>

inline void l4_sleep_forever(void) __attribute__((noreturn));

void l4_sleep_forever()
{
  l4_msgdope_t result;
  l4_umword_t dummy;
  for (;;) {
    l4_ipc_receive(L4_NIL_ID, L4_IPC_SHORT_MSG, &dummy, &dummy,
                   L4_IPC_NEVER, &result);
  }
}

inline void l4_touch_ro(const void*addr, unsigned size);

inline void l4_touch_ro(const void*addr, unsigned size)
{ 
  volatile const char *bptr, *eptr;

  bptr = (const char*)(((unsigned)addr) & L4_PAGEMASK);
  eptr = (const char*)(((unsigned)addr+size-1) & L4_PAGEMASK);
  for(;bptr<=eptr;bptr+=L4_PAGESIZE) {
    (void)(*bptr);
  }
}


/** Touch data areas to force mapping read-write */
inline void l4_touch_rw(const void*addr, unsigned size);

inline void l4_touch_rw(const void*addr, unsigned size)
{
  volatile char *bptr;
  volatile const char *eptr;
      
  bptr = (char*)(((unsigned)addr) & L4_PAGEMASK);
  eptr = (const char*)(((unsigned)addr+size-1) & L4_PAGEMASK);
  for(;bptr<=eptr;bptr+=L4_PAGESIZE) {
    char x = *bptr; 
    *bptr = x;
  }
}

#endif

