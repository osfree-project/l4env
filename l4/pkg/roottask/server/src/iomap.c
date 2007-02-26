/**
 * \file	roottask/server/src/iomap.c
 * \brief	I/O port resource handling
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#include <string.h>

#include "iomap.h"
#include "rmgr.h"

static owner_t __iomap[RMGR_IO_MAX];

void
iomap_init(void)
{
  memset(__iomap, O_RESERVED, RMGR_IO_MAX);
}

int
iomap_alloc_port(unsigned port, owner_t owner)
{
  owner_t *p = __iomap + port;

  if (*p == owner)
    return 1;			/* already allocated */
  if (*p != O_FREE)
    return 0;

  *p = owner;
  return 1;
}

owner_t
iomap_owner_port(unsigned port)
{
  return __iomap[port];
}

int
iomap_free_port(unsigned port, owner_t owner)
{
  owner_t *p = __iomap + port;

  if (*p == O_FREE)
    return 1;			/* page already free */
  if (*p != owner)
    return 0;			/* page not owned by owner */

  *p = O_FREE;
  return 1;
}
