/* $Id$ */
/*****************************************************************************/
/**
 * \file	l4io/server/src/main.c
 *
 * \brief	L4Env l4io I/O Server Base Module
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/ipc.h>
#include <l4/util/getopt.h>
#include <l4/rmgr/librmgr.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/generic_io/types.h>	/* IO's types */
#include <l4/generic_io/generic_io-server.h>	/* FLICK IPC interface */
#include <l4/oskit10_l4env/support.h>

/* OSKit includes */
#include <stdio.h>
#include <stdlib.h>

/* local includes */
#include "io.h"
#include "__config.h"
#include "__macros.h"

/* I/O server module interfaces */
#include "res.h"
#include "pci.h"
#include "jiffies.h"
#include "omega0lib.h"

/*****************************************************************************
 *** global vars
 *****************************************************************************/

/** I/O info page 
 * \ingroup grp_misc */
l4io_info_t io_info;

/** logging tag */
char LOG_tag[9] = IO_NAMES_STR;

/** max number of threads */
const int l4thread_max_threads = IO_MAX_THREADS;

/*****************************************************************************
 *** module vars
 *****************************************************************************/

/** I/O server claimed resources and root for client list
 *\ingroup grp_misc */
static io_client_t io_self;

/** I/O server claimed resources and root for client list
 *\ingroup grp_irq */
static int io_noirq = 0;

/*****************************************************************************/
/**
 * \name Miscellaneous Services (IPC interface)
 *
 * Client registry and special services.
 * @{ */
/*****************************************************************************/

/*****************************************************************************/
/** Client Registration.
 * \ingroup grp_misc
 *
 * \param  request	FLICK request structure
 * \param  type		client info
 *
 * \retval _ev		exception vector (unused)
 *
 * \return 0 on success, negative error code otherwise
 *
 * Register client (driver server).
 * I/O keeps a list of registered clients and only these will be served.
 */
/*****************************************************************************/
l4_int32_t 
l4_io_server_register_client(sm_request_t * request, l4_io_drv_t type,
			     sm_exc_t * _ev)
{
  io_client_t *c, *p;
  l4io_drv_t *drv = (l4io_drv_t *) & type;

  /* some memory for new list element */
  if (!(c = malloc(sizeof(io_client_t))))
    {
      ERROR("getting %d bytes of mem", sizeof(io_client_t));
      return -L4_ENOMEM;
    }

  /* init and enqueue */
  c->next = NULL;
  c->c_l4id = request->client_tid;
  c->drv = *drv;

#if DEBUG_REGDRV
  DMSG("registering "IdFmt" {%x, %x, %x}\n",
       IdStr(c->c_l4id), (unsigned char) c->drv.src,
       (unsigned char) c->drv.dsi, (unsigned char) c->drv.class);
#endif

  p = &io_self;
  for (;;)
    {
      if (!p->next)
	{
	  p->next = c;
	  break;
	}
      p = p->next;
    }

  return 0;
}

/*****************************************************************************/
/** Client Unregistering.
 * \ingroup grp_misc
 *
 * \param  request	FLICK request structure
 *
 * \retval _ev		exception vector (unused)
 *
 * \return 0 on success, negative error code otherwise
 *
 * Unregister client (driver server).
 * I/O keeps a list of registered clients and only these will be served.
 *
 * \krishna At this point we have to keep track of up-to-date resource
 * allocation info -> all allocated resources should be freed on
 * unregistering. (just I don't mind about this at this state, later driver
 * replacing will be in scope of this)
 *
 * \todo implement if appropriate; otherwise remove from IDL too
 */
/*****************************************************************************/
l4_int32_t 
l4_io_server_unregister_client(sm_request_t * request, sm_exc_t * _ev)
{
#if DEBUG_REGDRV
  DMSG("unregistering "IdFmt"\n", IdStr(request->client_tid));
#endif

  return -L4_ESKIPPED;
}

/*****************************************************************************/
/** Request mapping of I/O's info page.
 *
 * \param  request	FLICK request structure
 *
 * \retval info		L4 fpage for I/O info
 * \retval _ev		exception vector (unused)
 *
 * \return 0 on success, negative error code otherwise
 *
 * One fpage will be sent to the clients using the standard L4 fpage
 * types/mechanisms.
 *
 * \todo check registration on info page mapping
 */
/*****************************************************************************/
l4_int32_t 
l4_io_server_map_info(sm_request_t * request,
		      l4_snd_fpage_t * info, sm_exc_t * _ev)
{
  io_client_t *c;

  c = (io_client_t *) flick_server_get_local(request);

  /* mapping _after_ register */

  /* build send fpage */
  info->snd_base = 0;		/* hopefully no hot spot required */
  info->fpage = l4_fpage((l4_addr_t) & io_info,
			 L4_LOG2_PAGESIZE, L4_FPAGE_RO, L4_FPAGE_MAP);
#if DEBUG_MAP
  DMSG("sending info fpage {0x%08x, 0x%08x}\n",
       info->fpage.fp.page << 12, 1 << info->fpage.fp.size);
#endif

  /* done */
  return 0;
}

/** @} */
/*****************************************************************************/
/** Info initialization.
 *
 * \return 0 (at this state no errors may happen)
 *
 * Setup I/O info page values:
 *	- magic number
 *	- jiffies
 */
/*****************************************************************************/
static int io_info_init(void)
{
  io_info.magic = L4IO_INFO_MAGIC;
  io_info.jiffies = 0;
  io_info.hz = IOJIFFIES_HZ;

  return 0;
}

/*****************************************************************************/
/**
 * \name Heart of I/O
 *
 * Entry point (main) and infinite IPC server loop.
 * @{ */
/*****************************************************************************/

/*****************************************************************************/
/** I/O server IPC request loop.
 *
 * We act as FLICK server serving IPC requests and _never_ return to main()
 *
 * \krishna Hmm, is it clever to reference io_self in request?
 *
 * \krishna Do we need FLICK receive timeout?
 *
 * \todo design some lookup macros/functions for traversing our lists
 */
/*****************************************************************************/
static void io_loop(void)
{
  l4_msgdope_t result;

  sm_request_t request;		/* IPC request structure */
  l4_ipc_buffer_t ipc_buf;	/* IPC request buffer */

  flick_init_request(&request, &ipc_buf);

  flick_server_set_local(&request, (void *) &io_self);

  while (1)
    {
      result = flick_server_wait(&request);
      while (!L4_IPC_ERROR(request.ipc_status))
	{
	  switch (l4_io_server(&request))
	    {
	    case DISPATCH_ACK_SEND:
	      result = flick_server_reply_and_wait(&request);
	      break;
	    default:
	      printf("flick communication error");
	    }
	}
#if 0
      /* receive timeout */
      if (L4_IPC_ERROR(result) == L4_IPC_RETIMEOUT)
	{
	}
#endif
    }

  Panic("Left _infinite_ I/O server loop.");
}

/*****************************************************************************/
/** do_args.
 * command line parameter handling
 */
/*****************************************************************************/
static void do_args(int argc, char *argv[])
{
  char c;

  static int long_check;
  static int long_optind;
  static struct option long_options[] =
  {
    {"noirq", no_argument, &long_check, 1},
    {"include", required_argument, &long_check, 2},
    {"exclude", required_argument, &long_check, 3},
    {0, 0, 0, 0}
  };
  
  /* read command line arguments */
  optind = 0;
  long_optind = 0;
  while (1)
    {
      c = getopt_long_only(argc, argv, "", long_options, &long_optind);

      if (c == -1)
	break;

      switch (c)
	{
	case 0:		/* long option */
	  switch (long_check)
	    {
	    case 1:		/* debug jiffies */
	      io_noirq = 1;
	      Msg("Disabling internal IRQ handling.\n");
	      break;
	    case 2:
	      if(add_device_inclusion(optarg)){
		  Msg("invalid device:vendor string \"%s\"\n", optarg);
	      }
	      break;
	    case 3:
	      if(add_device_exclusion(optarg)){
		  Msg("invalid device:vendor string \"%s\"\n", optarg);
	      }
	      break;
	    default:
	      /* ignore unknown */
	      break;
	    }
	  break;
	default:
	  /* ignore unknown */
	  break;
	}
    }
}

/*****************************************************************************/
/** Main of I/O server.
 *
 * main() function
 *
 * \todo add some command line params to main if appropriate:
 *	- reserved resources
 */
/*****************************************************************************/
int main(int argc, char *argv[])
{
  int error;

  /* global init stuff */
  rmgr_init();
  OSKit_libc_support_init(1024 * 1024);

  do_args(argc, argv);

  /* init I/O info page */
  if ((error = io_info_init()))
    {
      ERROR("I/O info page initialization failed (%d)", error);
      return error;
    }

  /* setup self structure */
  io_self.next = NULL;
  io_self.c_l4id = l4thread_l4_id(l4thread_myself());
  strcpy(io_self.name, "IO SERVER");
  io_self.drv = L4IO_DRV_INVALID;

  /* start jiffies counter */
  if ((error = io_jiffies_init()))
    {
      ERROR("jiffies initialization failed (%d)", error);
      return error;
    }

  /* init submodules */
  if ((error = io_res_init(&io_self)))
    {
      ERROR("res initialization failed (%d)\n", error);
      return error;
    }
  if ((error = io_pci_init()))
    {
      ERROR("pci initialization failed (%d)\n", error);
      return error;
    }
  /* skip irq handling on demand */
  if (!io_noirq)
    {
      if ((error = OMEGA0_init()))
	{
	  ERROR("omega0 initialization failed (%d)\n", error);
	  return error;
	}
      io_info.omega0 = 1;
    }

  /* DEBUGGING */
  //      list_res();

  /* we are up -> register at names */
  if (!names_register(IO_NAMES_STR))
    {
      ERROR("can't register at names");
      return -L4_ENOTFOUND;
    }

  /* go looping */
  io_loop();

  exit(0);
}

/** @} */
