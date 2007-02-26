/**
 * \file	roottask/server/src/small.h
 * \brief	small address space resource handling
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#ifndef SMALL_H
#define SMALL_H

#include "types.h"

void    small_init(void);
int     small_alloc(unsigned smallno, owner_t owner);
int     small_free(unsigned smallno, owner_t owner);
owner_t small_owner(unsigned smallno);

#endif
