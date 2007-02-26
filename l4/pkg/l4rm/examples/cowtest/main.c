/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/examples/cowtest/main.c
 * \brief  Copy-on-write test. 
 *
 * \date   05/27/2000 
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 include */
#include <l4/log/l4log.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/rand.h>
#include <l4/util/rdtsc.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/env/env.h>

#include <stdio.h>

int main(void)
{
  int ret;
  l4dm_dataspace_t ds1,ds2,ds3;
  volatile char *cp;
  volatile char c1, c2, c3, c4;
  l4_threadid_t dm_id;
  l4_addr_t addr1,addr2,addr3;

  LOG_init("cowtest");
  
  /***************************************************************************
   * Region mapper test                                                      *
   ***************************************************************************/

  /* get dm id */
  dm_id = l4env_get_default_dsm();
  if (l4_is_invalid_id(dm_id))
    {
      printf("dm not found\n");
      enter_kdebug("-");
    }

  printf("dm: %x.%x\n",dm_id.id.task,dm_id.id.lthread);

  /* open new ds 1 */
  if ((ret = l4dm_mem_open(dm_id,1000,0,0,"test",&ds1)))
    {
      printf("error allocating dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("ds1 = %d at %x.%x\n",
      ds1.id,ds1.manager.id.task,ds1.manager.id.lthread);

  addr1 = 0x10000000;
  if ((ret = l4rm_attach_to_region(&ds1,(void *)addr1,1000,0,L4DM_RW)))
    {
      printf("error attaching dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("attached ds1 to region at 0x%08x\n",addr1);

  /* open new ds 2 by copying ds1 (without copy on write) */
  if ((ret = l4dm_copy(&ds1,0,"ds 2",&ds2)))
    {
      printf("error allocating dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("ds2 = %d at %x.%x\n",
      ds2.id,ds2.manager.id.task,ds2.manager.id.lthread);

  addr2 = 0x10010000;
  if ((ret = l4rm_attach_to_region(&ds2,(void *)addr2,1000,0,L4DM_RW)))
    {
      printf("error attaching dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("attached ds2 to region at 0x%08x\n",addr2);

  /* open new ds 3 by copying ds1 (with copy on write) */
  if ((ret = l4dm_copy(&ds1,L4DM_COW,"ds 3",&ds3)))
    {
      printf("error allocating dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("ds3 = %d at %x.%x\n",
      ds3.id,ds3.manager.id.task,ds3.manager.id.lthread);

  addr3 = 0x10020000;
  if ((ret = l4rm_attach_to_region(&ds3,(void *)addr3,1000,0,L4DM_RW)))
    {
      printf("error attaching dataspace: %d\n",ret);
      enter_kdebug("???");
    }
  printf("attached ds3 to region at 0x%08x\n",addr3);

  enter_kdebug("stop");

  cp = (volatile char *)0x10000000;
  *cp = 'A';
  
  cp = (volatile char *)0x10000000; 
  c1 = *cp++; c2 = *cp++; c3 = *cp++; c4 = *cp++;
  printf("(0x10000000) = %02d %02d %02d %02d\n", c1, c2, c3, c4);
  cp = (volatile char *)0x10010000; 
  c1 = *cp++; c2 = *cp++; c3 = *cp++; c4 = *cp++;
  printf("(0x10010000) = %02d %02d %02d %02d\n", c1, c2, c3, c4);
  cp = (volatile char *)0x10020000; 
  c1 = *cp++; c2 = *cp++; c3 = *cp++; c4 = *cp++;
  printf("(0x10020000) = %02d %02d %02d %02d\n", c1, c2, c3, c4);

  enter_kdebug("stop");

  cp = (volatile char *)0x10010000;
  *cp = 'B';

  cp = (volatile char *)0x10000000; 
  c1 = *cp++; c2 = *cp++; c3 = *cp++; c4 = *cp++;
  printf("(0x10000000) = %02d %02d %02d %02d\n", c1, c2, c3, c4);
  cp = (volatile char *)0x10010000; 
  c1 = *cp++; c2 = *cp++; c3 = *cp++; c4 = *cp++;
  printf("(0x10010000) = %02d %02d %02d %02d\n", c1, c2, c3, c4);
  cp = (volatile char *)0x10020000; 
  c1 = *cp++; c2 = *cp++; c3 = *cp++; c4 = *cp++;
  printf("(0x10020000) = %02d %02d %02d %02d\n", c1, c2, c3, c4);
  
  enter_kdebug("stop");

  cp = (volatile char *)0x10020001;
  *cp = 'C';

  cp = (volatile char *)0x10000000; 
  c1 = *cp++; c2 = *cp++; c3 = *cp++; c4 = *cp++;
  printf("(0x10000000) = %02d %02d %02d %02d\n", c1, c2, c3, c4);
  cp = (volatile char *)0x10010000; 
  c1 = *cp++; c2 = *cp++; c3 = *cp++; c4 = *cp++;
  printf("(0x10010000) = %02d %02d %02d %02d\n", c1, c2, c3, c4);
  cp = (volatile char *)0x10020000; 
  c1 = *cp++; c2 = *cp++; c3 = *cp++; c4 = *cp++;
  printf("(0x10020000) = %02d %02d %02d %02d\n", c1, c2, c3, c4);
 
  enter_kdebug("stop");
  
  cp = (volatile char *)0x10020002;
  *cp = 'D';

  cp = (volatile char *)0x10000000; 
  c1 = *cp++; c2 = *cp++; c3 = *cp++; c4 = *cp++;
  printf("(0x10000000) = %02d %02d %02d %02d\n", c1, c2, c3, c4);
  cp = (volatile char *)0x10010000; 
  c1 = *cp++; c2 = *cp++; c3 = *cp++; c4 = *cp++;
  printf("(0x10010000) = %02d %02d %02d %02d\n", c1, c2, c3, c4);
  cp = (volatile char *)0x10020000; 
  c1 = *cp++; c2 = *cp++; c3 = *cp++; c4 = *cp++;
  printf("(0x10020000) = %02d %02d %02d %02d\n", c1, c2, c3, c4);

  enter_kdebug("stop");

  printf("main done\n");

  return 0;
}
