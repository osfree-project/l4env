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
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>

#include <l4/rmgr/librmgr.h>

#include <l4/util/getopt.h>		/* from libl4util */

#include <l4/names/libnames.h>
#include <l4/util/util.h>

int
main(int argc, char* argv[])
{
  l4_threadid_t id;
  char		buffer[1024];
  int           i;

  outstring("Registering ABCGEFG ");
  if (names_register("ABCGEFG"))
    outstring("OK\r\n");
  else
    outstring("FAILED!!!\r\n");

  outstring("Registering ABCGEFG ");
  if (names_register("ABCGEFG"))
    outstring("OK\r\n");
  else
    outstring("FAILED!!! (but expected)\r\n");

  outstring("Registering ABCGEFG2 ");
  if (names_register("ABCGEFG2"))
    outstring("OK\r\n");
  else
    outstring("FAILED!!!\r\n");







  outstring("Querying ABCGEFG ");
  if (names_query_name("ABCGEFG", &id))
    {
      outstring(" -> ");
      outdec(id.id.task);
      outstring(".");
      outdec(id.id.lthread);
      outstring("\r\n");
    }
  else
    outstring("FAILED!!!\r\n");



  outstring("Querying names ");
  if (names_query_name("names", &id))
    {
      outstring(" -> ");
      outdec(id.id.task);
      outstring(".");
      outdec(id.id.lthread);
      outstring("\r\n");
    }
  else
    outstring("FAILED!!!\r\n");


  outstring("Querying ABCGEFG2 ");
  if (names_query_name("ABCGEFG2", &id))
    {
      outstring(" -> ");
      outdec(id.id.task);
      outstring(".");
      outdec(id.id.lthread);
      outstring("\r\n");
    }
  else
    outstring("FAILED!!!\r\n");

  outstring("Querying 5.0 ");
  id = l4_myself(); id.id.task = 5;
  if (names_query_id(id, buffer, sizeof(buffer)))
    {
      outstring(" -> ");
      outdec(id.id.task);
      outstring(".");
      outdec(id.id.lthread);
      outchar(' ');
      outstring(buffer);
      outstring("\r\n");
    }
  else
    outstring("FAILED!!!\r\n");


  outstring("Query all: ");
  for (i = 0; i < NAMES_MAX_ENTRIES; i++)
    {
      if (names_query_nr(i, buffer, sizeof(buffer), &id))
	{
	  if (i)
	    outstring(", ");
	  outstring(buffer);
	  outstring("(");
	  outdec(id.id.task);
	  outstring(".");
	  outdec(id.id.lthread);
	  outstring(")");
	}
    }

  outstring("\r\n");






  outstring("Unregistering ABCGEFG ");
  if (names_unregister("ABCGEFG"))
    outstring("OK\r\n");
  else
    outstring("FAILED!!!\r\n");

  outstring("Querying ABCGEFG ");
  if (names_query_name("ABCGEFG", &id))
    {
      outstring(" -> ");
      outdec(id.id.task);
      outstring(".");
      outdec(id.id.lthread);
      outstring("\r\n");
    }
  else
    outstring("FAILED!!! (but expected)\r\n");

  outstring("Unregistering ABCGEFG ");
  if (names_unregister("ABCGEFG"))
    outstring("OK\r\n");
  else
    outstring("FAILED!!! (but expected)\r\n");


  outstring("Requesting dump from names:\r\n");
  names_dump();

  

  outstring(__FILE__" Done.\r\n");
  if (names_waitfor_name("DEMO2", &id, 8000))
    outstring(__FILE__"waitfor OK\r\n");
  else
    outstring(__FILE__"waitfor ~OK\r\n");
  l4_sleep(-1);
  outstring(__FILE__" end\r\n");
  return 0;
};
