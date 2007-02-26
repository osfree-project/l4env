/* $Id$ */
/**
 * \file	exec/server/src/exc.cc
 * \brief	Some helper functions to deal without libstdc++
 *
 * \date	10/30/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */

#include <stdio.h>
#include <malloc.h>

#include "exc.h"
#include "assert.h"

/** Init values of the infopage which are important to us. */
void
exc_init_env(int id, l4env_infopage_t *env)
{
  env->id = id;
  env->section_num = 0;
}

/** libstdc++ new emulator */
void*
operator new(unsigned int size)
{
  void *ptr = malloc(size);
  if (!ptr)
    Error("malloc(%d) failed", size);
  
  return ptr;
}

/** libstdc++ delete emulator */
void
operator delete(void *addr)
{
  free(addr);
}

#ifdef __GNUC__
#if __GNUC__ < 3
/** libstdc++ __pure_virtual backcall */
extern "C" void
__pure_virtual(void)
{
  Panic("Pure virtual method called");
}
#else
/** libstdc++ __pure_virtual backcall */
extern "C" void
__cxa_pure_virtual(void)
{
  Panic("Pure virtual method called");
}
#endif
#endif
