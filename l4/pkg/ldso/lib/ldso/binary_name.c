/*!
 * \file   ldso/lib/ldso/binary_name.c
 * \brief  Determine the name of the binary.
 *
 * \date   01/2006
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 */
/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/util/mb_info.h>
#include <l4/util/mbi_argv.h>

#include "infopage.h"
#include "binary_name.h"

const char*
binary_name(char *buffer, l4_size_t size)
{
  const char *n = 0;
  int i;

  if (l4util_argv[0])
    n = l4util_argv[0];

  else
    {
      l4util_mb_info_t *mbi;

      if (!global_env)
	return 0;

      if (!(mbi = (l4util_mb_info_t*)global_env->addr_mb_info))
	return 0;

      if (!(mbi->flags & L4UTIL_MB_CMDLINE))
	return 0;

      if (!(n = (char*)mbi->cmdline))
	return 0;
    }

  for (i=0; i<size-1 && n[i] && n[i]!=' ' && n[i]!='\t'; i++)
    buffer[i] = n[i];
  buffer[i] = '\0';

  return buffer;
}
