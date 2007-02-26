/**
 * \file	roottask/server/src/vm.h
 * \brief	memory resource handling
 *
 * \date	10/12/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#ifndef VM_H
#define VM_H


#include <l4/sys/types.h>
#include "types.h"

void      vm_add(unsigned task,
		 l4_addr_t vm_start, l4_addr_t vm_end, l4_addr_t offest);
int       vm_find(unsigned task, l4_addr_t addr);
l4_addr_t vm_get_offset(int i);

#endif
