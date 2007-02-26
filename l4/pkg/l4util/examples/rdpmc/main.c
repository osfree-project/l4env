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
  const char *pmcdiffstr = 0, *tscdiffstr = 0;
  unsigned pmc;
  unsigned long long ll, ll_pmc = 0, ll_tsc = 0;

  if ((error = parse_cmdline(&argc, &argv,
		'f', "fiasco_log", "create an Fiasco tracebuffer entry",
		PARSE_CMD_SWITCH, 1, &do_log,
		'p', "pmc", "show perfctr with specified number",
		PARSE_CMD_INT, 0xffffffff, &pmc,
		't', "tsc", "show time stamp counter",
		PARSE_CMD_SWITCH, 1, &show_tsc,
		'P', "pmcdiff", "show difference to previous perfctr value",
		PARSE_CMD_STRING, "", &pmcdiffstr,
		'T', "tscdiff", "show difference to previous tsc value",
		PARSE_CMD_STRING, "", &tscdiffstr,
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
      asm volatile ("rdpmc" : "=A" (ll) : "c" (pmc));
      ll &= perfctr_mask;
      if (pmcdiffstr && *pmcdiffstr)
	{
	  ll_pmc = (ll - strtoll(pmcdiffstr, NULL, 0)) & perfctr_mask;
	  printf("%llu ", ll_pmc);
	}
      else
	printf("%llu ", ll);
    }

  if (show_tsc)
    {
      asm volatile ("rdtsc" : "=A" (ll));
      if (tscdiffstr && *tscdiffstr)
	{
	  ll_tsc = ll - strtoll(tscdiffstr, NULL, 0);
	  printf("%llu ", ll_tsc);
	}
      else
	printf("%llu ", ll);
    }

  if (ll_tsc && ll_pmc)
    {
      ll = ll_pmc * 1000 / ll_tsc;
      printf("%llu.%llu", ll/10, ll-(ll/10)*10);
    }

  if (do_log)
    fiasco_tbuf_log("LOG ENTRY");
  putchar('\n');
  return 0;
}
