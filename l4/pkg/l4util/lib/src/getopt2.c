/**
 * \file   l4util/lib/src/getopt2.c
 * \brief  initialize argc/argv from multiboot structure
 *
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <string.h>

#include <l4/util/mbi_argv.h>
#include <l4/crtx/crt0.h>

#define MAXARGC 20
#define MAXENVC 30
static char argbuf[1024];

char *l4util_argv[MAXARGC];
int  l4util_argc = 0;

#define isspace(c) ((c)==' '||(c)=='\t'||(c)=='\r'||(c)=='\n')

static void
parse_args(char *argbuf)
{
  char *cp;

  /* make l4util_argc, l4util_argv */
  l4util_argc = 0;
  cp = argbuf;
  while (*cp && l4util_argc < MAXARGC-1)
    {
      while (*cp && isspace(*cp))
	cp++;

      if (*cp)
	{
	  l4util_argv[l4util_argc++] = cp;
	  while (*cp && !isspace(*cp))
	    cp++;

	  if (*cp)
	    *cp++ = '\0';
	}
    }
  l4util_argv[l4util_argc] = (void*) 0;
}

static void
arg_init(char* cmdline)
{
  if (cmdline)
    {
      strncpy(argbuf, cmdline,
	      sizeof(argbuf) < strlen(cmdline) ?
	      sizeof(argbuf) : 1+strlen(cmdline));
      parse_args(cmdline);
    }
}

void 
l4util_mbi_to_argv(l4_mword_t flag, l4util_mb_info_t *mbi)
{
  if (flag == L4UTIL_MB_VALID
      && mbi && (mbi->flags & L4UTIL_MB_CMDLINE))
    arg_init((char*) mbi->cmdline);
}

