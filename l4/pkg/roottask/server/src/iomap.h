/**
 * \file	roottask/server/src/iomap.h
 * \brief	I/O port resource handling
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#ifndef IO_MAP_H
#define IO_MAP_H

#include "types.h"

void    iomap_init(void);
int     iomap_alloc_port(unsigned port, owner_t owner);
owner_t iomap_owner_port(unsigned port);
int     iomap_free_port(l4_addr_t address, owner_t owner);

#endif
