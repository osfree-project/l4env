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


//  rmgr_init();
  
  kd_display("Registering ABCGEFG ");
  if (names_register("ABCGEFG"))
    kd_display("OK\\r\\n");
  else
    kd_display("FAILED!!!\\r\\n");

  kd_display("Registering ABCGEFG ");
  if (names_register("ABCGEFG"))
    kd_display("OK\\r\\n");
  else
    kd_display("FAILED!!!\\r\\n");

  kd_display("Registering ABCGEFG2 ");
  if (names_register("ABCGEFG2"))
    kd_display("OK\\r\\n");
  else
    kd_display("FAILED!!!\\r\\n");







  kd_display("Querying ABCGEFG ");
  if (names_query_name("ABCGEFG", &id))
    {
      kd_display(" -> ");
      outdec(id.id.task);
      kd_display(".");
      outdec(id.id.lthread);
      kd_display("\\r\\n");
    }
  else
    kd_display("FAILED!!!\\r\\n");



  kd_display("Querying names ");
  if (names_query_name("names", &id))
    {
      kd_display(" -> ");
      outdec(id.id.task);
      kd_display(".");
      outdec(id.id.lthread);
      kd_display("\\r\\n");
    }
  else
    kd_display("FAILED!!!\\r\\n");


  kd_display("Querying ABCGEFG2 ");
  if (names_query_name("ABCGEFG2", &id))
    {
      kd_display(" -> ");
      outdec(id.id.task);
      kd_display(".");
      outdec(id.id.lthread);
      kd_display("\\r\\n");
    }
  else
    kd_display("FAILED!!!\\r\\n");

  kd_display("Querying 5.0 ");
  id = l4_myself(); id.id.task = 5;
  if (names_query_id(id, buffer, sizeof(buffer)))
    {
      outdec(id.id.task);
      kd_display(".");
      outdec(id.id.lthread);
      kd_display(" -> ");
      outstring(buffer);
      kd_display("\\r\\n");
    }
  else
    kd_display("FAILED!!!\\r\\n");







  kd_display("Unregistering ABCGEFG ");
  if (names_unregister("ABCGEFG"))
    kd_display("OK\\r\\n");
  else
    kd_display("FAILED!!!\\r\\n");

  kd_display("Querying ABCGEFG ");
  if (names_query_name("ABCGEFG", &id))
    {
      kd_display(" -> ");
      outdec(id.id.task);
      kd_display(".");
      outdec(id.id.lthread);
      kd_display("\\r\\n");
    }
  else
    kd_display("FAILED!!!\\r\\n");

  kd_display("Unregistering ABCGEFG ");
  if (names_unregister("ABCGEFG"))
    kd_display("OK\\r\\n");
  else
    kd_display("FAILED!!!\\r\\n");

  kd_display(__FILE__" Fertig\\r\\n");
  if (names_waitfor_name("DEMO2", &id, 8000))
    kd_display(__FILE__"waitfor OK\\r\\n");
  else
    kd_display(__FILE__"waitfor ~OK\\r\\n");
  l4_sleep(-1);
  kd_display(__FILE__" end\\r\\n");
  return 0;
};
