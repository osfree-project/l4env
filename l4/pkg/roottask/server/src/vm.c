/**
 * \file	roottask/server/src/vm.c
 * \brief	memory resource handling
 *
 * \date	10/12/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/

#include <stdio.h>
#include "vm.h"

#define VM_MAX 32

static __vm_t __vm[VM_MAX];
static int size = 0;

void
vm_add(unsigned task, l4_addr_t vm_start, l4_addr_t vm_end,
       l4_addr_t offset)
{
  if (size == VM_MAX)
    {
      printf("WARNING: no space for vm region.");
    }

  __vm[size].task = task;
  __vm[size].vm_start = vm_start;
  __vm[size].vm_end = vm_end;
  __vm[size].offset = offset;
  size++;

  return;
}

int
vm_find(unsigned task, l4_addr_t addr)
{
  int i;

  for (i = 0; i < size; i++)
    {
      if (__vm[i].vm_start <= addr && __vm[i].vm_end > addr &&
	  __vm[i].task == task)
	return i;
    }

  return -1;
}

l4_addr_t
vm_get_offset(int i)
{
  printf("vm: %x, %x, %x, %x\n", __vm[i].task, __vm[i].vm_start,
         __vm[i].vm_end, __vm[i].offset);
  return __vm[i].offset;
}
