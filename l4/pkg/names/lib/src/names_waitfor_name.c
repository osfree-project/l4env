/*!
 * \file   names/lib/src/names_waitfor_name.c
 * \brief  Implementation of names_waitfor_name()
 *
 * \date   05/27/2003
 * \author Uwe Dannowski <Uwe.Dannowski@ira.uka.de>
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/sys/syscalls.h>
#include <l4/util/util.h>
#include <l4/names/libnames.h>

/*!\brief Repeatedly query for a given name
 * \ingroup clientapi
 *
 * \param  name		0-terminated name the ID should be returned for.
 * \param  id		thread ID will be stored here.
 * \param  timeout	timeout in ms,
 *
 * \retval 0		Error. The name was not registered within the timeout.
 * \retval !=0		Success.
 *
 * The name service is repeatedly queried for the string name. If the
 * name gets registered before timeout is over, the associated
 * thread_id is copied into the buffer referenced by id.
 */
int
names_waitfor_name(const char* name, l4_threadid_t* id,
		   const int timeout)
{
  int		ret;
  signed int	rem = timeout;
  signed int	to = 0;

  if (rem)
    to = 10;
  do
    {
      ret = names_query_name(name, id);

      if ((rem == to) || (ret))
	return ret;
      l4_sleep(to);

      rem -= to;
      if (to < 100)
	to += to;
      if (to > rem)
	to = rem;
    }
  while (486);
}
