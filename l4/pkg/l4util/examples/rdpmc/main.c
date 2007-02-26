/**
 * \file   l4util/examples/rdpmc/main.c
 * \brief  Simple Linux user mode program for reading a performance counter
 *
 * \date   Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>
#include <stdlib.h>
#include <l4/util/parse_cmd.h>
#include <l4/sys/ktrace.h>

const unsigned long long perfctr_mask = 0x000000ffffffffffULL;

int
main(int argc, const char *argv[])
{
  int error, show_tsc = 0, do_log = 0;
  const char *diffstr = 0;
  unsigned pmc;

  if ((error = parse_cmdline(&argc, &argv,
		'd', "diff", "show difference to previous perfctr value",
		PARSE_CMD_STRING, "", &diffstr,
		'f', "fiasco_log", "create an Fiasco tracebuffer entry",
		PARSE_CMD_SWITCH, 1, &do_log,
		'p', "pmc", "show perfctr with specified number",
		PARSE_CMD_INT, 0xffffffff, &pmc,
		't', "tsc", "show time stamp counter",
		PARSE_CMD_SWITCH, 1, &show_tsc,
		0)))
    {
      switch (error)
	{
	case -1: printf("Bad parameter for parse_cmdline()\n"); break;
	case -2: printf("Out of memory in parse_cmdline()\n"); break;
	case -4: return 1;
	default: printf("Error %d in parse_cmdline()\n", error); break;
	}
      return 1;
    }

  if (pmc != 0xffffffff)
    {
      unsigned long long ll;

      asm volatile ("rdpmc" : "=A" (ll) : "c" (pmc));
      ll &= perfctr_mask;
      printf("%llu ", ll);
      if (diffstr && *diffstr)
	{
	  unsigned long long lllast = strtoll(diffstr, NULL, 0);
	  
	  ll -= lllast;
	  ll &= perfctr_mask;
	  printf("%llu ", ll);
	}
    }

  if (show_tsc)
    {
      unsigned long long ll;

      asm volatile ("rdtsc" : "=A" (ll));
      printf("%llu ", ll);
    }

  if (do_log)
    fiasco_tbuf_log("LOG ENTRY");
  putchar('\n');
  return 0;
}
