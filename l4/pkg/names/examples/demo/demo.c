/*!
 * \file   names/examples/demo/demo.c
 * \brief  Demo showing the basic functions of names
 *
 * \date   05/27/2003
 * \author Uwe Dannowski <Uwe.Dannowski@ira.uka.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdio.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>

#include <l4/rmgr/librmgr.h>

#include <l4/util/getopt.h> /* from libl4util */
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>

#include <l4/names/libnames.h>

int
main(int argc, char* argv[])
{
  l4_threadid_t id;
  char          buffer[1024];

  printf("Waiting for dm_phys to register... ");
  while (names_waitfor_name("DM_PHYS", &id, 1000) == 0)
    /* Do nothing */
    ;
  printf("OK\n");


  printf("Registering ABCGEFG ");
  if (names_register("ABCGEFG"))
    printf("OK\n");
  else
    printf("FAILED!!!\n");

  printf("Registering ABCGEFG ");
  if (names_register("ABCGEFG"))
    printf("OK\n");
  else
    printf("FAILED!!! (but expected)\n");


  printf("Registering ABCGEFG2 ");
  if (names_register("ABCGEFG2"))
    printf("OK\n");
  else
    printf("FAILED!!!\n");


  printf("Querying ABCGEFG ");
  if (names_query_name("ABCGEFG", &id))
    printf(" -> "l4util_idfmt"\n", l4util_idstr(id));
  else
    printf("FAILED!!!\n");


  printf("Querying names ");
  if (names_query_name("names", &id))
    printf(" -> "l4util_idfmt"\n", l4util_idstr(id));
  else
    printf("FAILED!!!\n");


  printf("Querying ABCGEFG2 ");
  if (names_query_name("ABCGEFG2", &id))
    printf(" -> "l4util_idfmt"\n", l4util_idstr(id));
  else
    printf("FAILED!!!\n");


  printf("Querying 5.0 ");
  id = l4_myself(); id.id.task = 5;
  if (names_query_id(id, buffer, sizeof(buffer)))
    printf(" -> "l4util_idfmt" %s\n", l4util_idstr(id), buffer);
  else
    printf("FAILED!!!\n");


  /* XXX: Removed in order to make the ptest run correctly. */
#if 0
  printf("Query all: ");
  for (i = 0; i < NAMES_MAX_ENTRIES; i++)
    {
      if (names_query_nr(i, buffer, sizeof(buffer), &id))
        {
          if (i)
            printf(", ");
          printf("%s ("l4util_idfmt")", buffer, l4util_idstr(id));
        }
    }
  printf("\n");
#endif

  printf("Unregistering ABCGEFG ");
  if (names_unregister("ABCGEFG"))
    printf("OK\n");
  else
    printf("FAILED!!!\n");

  printf("Querying ABCGEFG ");
  if (names_query_name("ABCGEFG", &id))
    printf(" -> "l4util_idfmt"\n", l4util_idstr(id));
  else
    printf("FAILED!!! (but expected)\n");

  printf("Unregistering ABCGEFG ");
  if (names_unregister("ABCGEFG"))
    printf("OK\n");
  else
    printf("FAILED!!! (but expected)\n");

  printf("Requesting dump from names:\n");
  names_dump();

  printf("Done.\n");
  if (names_waitfor_name("DEMO2", &id, 8000))
    printf("waitfor OK\n");
  else
    printf("waitfor FAILED (but expected)\n");
  l4_sleep(-1);
  printf("end\n");
  return 0;
};
