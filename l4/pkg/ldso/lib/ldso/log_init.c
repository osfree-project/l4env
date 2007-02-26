/*!
 * \file   ldso/lib/ldso/log_init.c
 * \brief  Initialize the LOG_tag. Replaces log/lib/src/log_init.c.
 *
 * \date   10/2005
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 */
/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/log/l4log.h>
#include <l4/crtx/ctor.h>

#include "binary_name.h"

char LOG_tag[9] __attribute__((weak));
char LOG_tag[9] = "";

/* Initialize the LOG_tag after the cmdline has been parsed, but before
 * the l4env or other systems come up.
 */
/*!\brief Set the LOG tag.
 *
 * This function should be called by the startup-code, prior to any output
 * of the loglib (if used) or the thread-creation using the thread package.
 * If l4util_progname is empty, it will be filled with the first 8 chars of
 * the last portion of the program name.
 *
 * With L4Env, this function is called after initializing l4util's argc/argv.
 * It is called via constructors a second time before calling main().
 * Without L4Env and the __main() from l4util, this function is called
 * as constructor after initializing argc/argv.
 *
 * Prior to be called by the startup-code, this function is called from
 * _dl_start().
 */

void
LOG_setup_tag(void)
{
  if (LOG_tag[0] == 0)
    {
      char *d;
      const char *e;
      char buffer[128];
      const char *name = binary_name(buffer, sizeof(buffer));

      /* look for last '/' in name */
      for (e = name; *e!='\0' && *e!=' '; e++)
        if (*e == '/')
          name = e + 1;

      e = LOG_tag + sizeof(LOG_tag) - 1;
      for (d = LOG_tag; *name!='\0' && *name!=' ' && d < e; )
        *d++ = *name++;
      *d = 0;
    }
}
