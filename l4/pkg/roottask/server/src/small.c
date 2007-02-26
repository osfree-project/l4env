/**
 * \file	roottask/server/src/small.c
 * \brief	small address space resource handling
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#include <string.h>

#include "quota.h"
#include "rmgr.h"
#include "small.h"

static owner_t __small[RMGR_SMALL_MAX];

void
small_init(void)
{
  memset(__small, O_RESERVED, RMGR_SMALL_MAX);
}

int
small_alloc(unsigned smallno, owner_t owner)
{
  if (__small[smallno] == owner)
    return 1;
  if (__small[smallno] != O_FREE)
    return 0;

  if (! quota_alloc_small(owner, smallno))
    return 0;

  __small[smallno] = owner;
  return 1;
}

int
small_free(unsigned smallno, owner_t owner)
{
  if (__small[smallno] != owner && __small[smallno] != O_FREE)
    return 0;

  if (__small[smallno] != O_FREE)
    {
      quota_free_small(owner, smallno);
      __small[smallno] = O_FREE;
    }
  return 1;
}

owner_t
small_owner(unsigned smallno)
{
  return __small[smallno];
}
