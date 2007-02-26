/*!
 * \file   cpu_reserve/server/src/corba.c
 * \brief  Corba functions
 *
 * \date   08/25/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdlib.h>
#include <dice/dice.h>

void* CORBA_alloc(unsigned long size){
    return malloc(size);
}
void CORBA_free(void *ptr){
    return free(ptr);
}

