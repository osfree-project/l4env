/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4env/lib/src/startup_noenv.c
 * \brief  Task startup, noenv version
 *
 * \date   08/05/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * This version does not initializes the L4env services (l4rm, thread, 
 * semaphore, ...). It's intended to be used with servers which not use that 
 * services but want to use other functionality of the L4env library. Use 
 * crt0_l4env_noenv.S as entry code.
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

/* L4 includes */
#include <l4/env/env.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>

/*****************************************************************************
 *** external symbols
 *****************************************************************************/

/* external symbols */
extern int    _argc;
extern char * _argv[];

extern int main(int argc, char ** argv);

/*****************************************************************************
 *** L4Env functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Deliver L4 environment page
 *
 * Since this is a old-style L4 task, it has no L4 environment infopage
 * mapped in from the L4 Loader. But nevertheless, it could be started by
 * the compatibility mode of the L4 loader.
 */
/*****************************************************************************/ 
l4env_infopage_t*
l4env_get_infopage(void)
{
  /* deliver 0 since we have nothing to deliver */
  return NULL;
}

/*****************************************************************************/
/**
 * \brief Startup code.
 */
/*****************************************************************************/ 
void 
l4env_startup(void);

void 
l4env_startup(void)
{
  l4_umword_t dummy;
  l4_threadid_t preempter,pager;

  /* init some internal L4env stuff */
  preempter = pager = L4_INVALID_ID;
  l4_thread_ex_regs(l4_myself(), (l4_umword_t)-1, (l4_umword_t)-1,
		    &preempter, &pager, &dummy, &dummy, &dummy);
  l4env_set_sigma0_id(pager);

  /* start application main */
  main(_argc, (char**)(&_argv));
}

