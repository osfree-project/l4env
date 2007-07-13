/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/src/main.c
 * \brief  L4Env l4io I/O Server Base Module
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/ipc.h>
#include <l4/sigma0/kip.h>
#include <l4/util/getopt.h>
#include <l4/rmgr/librmgr.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/generic_io/types.h>                /* IO's types */
#include <l4/generic_io/generic_io-server.h>    /* IPC interface */

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
#include "events.h"
#include "omega0lib.h"

/*
 * global vars
 */

/** I/O info page
 * \ingroup grp_misc */
l4io_info_t io_info;

/** logging tag */
char LOG_tag[9] = IO_NAMES_STR;

/** heap */
l4_ssize_t l4libc_heapsize = 1 << 20;

/** configure thread library -- max number of threads */
const int l4thread_max_threads = IO_MAX_THREADS;

/** Configure thread library -- size of stack. Default is 64KB. Decrease
 *  this number because we have some threads. */
const l4_size_t l4thread_stack_size = 16 << 10;

static int cfg_events;            /* receive exit events            (default off) */
static int cfg_dev_list = 1;      /* list PCI devices at bootup     (default on)  */
static int cfg_token = CFG_STD;   /* runtime configration token for static cfg    */

/*
 * module vars
 */

/** I/O server claimed resources and root for client list
 *\ingroup grp_misc */
static io_client_t io_self;


/** \name Miscellaneous Services (IPC interface)
 *
 * Client registry and special services.
 * @{ */

/** Client Registration.
 * \ingroup grp_misc
 *
 * \param  _dice_corba_obj  DICE corba object
 * \param  type             client info
 *
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 *
 * Register client (driver server).
 * I/O keeps a list of registered clients and only these will be served.
 */
long
l4_io_register_client_component (CORBA_Object _dice_corba_obj,
                                 l4_io_drv_t type,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  io_client_t *c, *p;
  l4io_drv_t *drv = (l4io_drv_t *) & type;

  /* some memory for new list element */
  if (!(c = malloc(sizeof(io_client_t))))
    {
      LOGdL(DEBUG_ERRORS, "getting %ld bytes of mem", 
	  (unsigned long)sizeof(io_client_t));
      return -L4_ENOMEM;
    }

  /* init and enqueue */
  c->next = NULL;
  c->c_l4id = *_dice_corba_obj;
  c->drv = *drv;

  LOGd(DEBUG_REGDRV, "registering "l4util_idfmt" {%x, %x, %x}",
       l4util_idstr(c->c_l4id), (unsigned char) c->drv.src,
       (unsigned char) c->drv.dsi, (unsigned char) c->drv.drv_class);

  for (p = &io_self; p->next; p = p->next)
    ;
  p->next = c;

  return 0;
}

/** Client Unregistering.
 * \ingroup grp_misc
 *
 * \param  _dice_corba_obj  DICE corba object
 *
 * \retval _dice_corba_env  corba environment
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
 */
long
l4_io_unregister_client_component (CORBA_Object _dice_corba_obj,
                                   const l4_threadid_t *client,
                                   CORBA_Server_Environment *_dice_corba_env)
{
  io_client_t *c, *p, tmp;

  LOGd(DEBUG_REGDRV,
       "unregistering "l4util_idfmt" (called by "l4util_idfmt")", 
       l4util_idstr(*client), l4util_idstr(*_dice_corba_obj));

  if (!l4_tasknum_equal(*_dice_corba_obj, io_self.c_l4id) &&
      !l4_tasknum_equal(*_dice_corba_obj, *client))
    /* A client may not unregister another client */
    {
      LOGd(DEBUG_REGDRV, "=> not allowed");
      return -L4_EPERM;
    }

  tmp.c_l4id = *client;

  for (p = &io_self, c = io_self.next; c; p = c, c = c->next)
    {
      if (client_equal(c, &tmp))
        {
          p->next = c->next;
          return 0;
        }
    }

  LOGd(DEBUG_REGDRV, "=> not found");
  return -L4_ENOTFOUND;
}

/** Request mapping of I/O's info page.
 *
 * \param  _dice_corba_obj  DICE corba object
 *
 * \retval info             L4 fpage for I/O info
 * \retval _dice_corba_env  corba environment
 *
 * \return 0 on success, negative error code otherwise
 *
 * One fpage will be sent to the clients using the standard L4 fpage
 * types/mechanisms.
 *
 * \todo check registration on info page mapping
 */
long
l4_io_map_info_component (CORBA_Object _dice_corba_obj,
                          l4_snd_fpage_t *info,
                          CORBA_Server_Environment *_dice_corba_env)
{
  io_client_t *c;

  c = (io_client_t *) (_dice_corba_env->user_data);

  /* mapping _after_ register */

  /* build send fpage */
  info->snd_base = 0;  /* hopefully no hot spot required */
  info->fpage = l4_fpage((l4_addr_t) & io_info,
                         L4_LOG2_PAGESIZE, L4_FPAGE_RO, L4_FPAGE_MAP);

  LOGd(DEBUG_MAP, "sending info fpage {0x%08lx, 0x%08lx}",
       (unsigned long)info->fpage.fp.page << 12, 1UL << info->fpage.fp.size);

  /* done */
  return 0;
}

/** @} */
/** Info initialization.
 *
 * \return 0 (at this state no errors may happen)
 *
 * Setup I/O info page values:
 *  - magic number
 *  - jiffies
 */
static int io_info_init(void)
{
  io_info.magic = L4IO_INFO_MAGIC;
  io_info.jiffies = 0;
  io_info.hz = IOJIFFIES_HZ;

  return 0;
}

/** \name Heart of I/O
 *
 * Entry point (main) and infinite IPC server loop.
 * @{ */

/** I/O server IPC request loop.
 *
 * We act as DICE server serving IPC requests and _never_ return to main()
 *
 * \krishna Hmm, is it clever to reference io_self in request?
 *
 * \krishna Do we need receive timeouts?
 *
 * \todo design some lookup macros/functions for traversing our lists
 */
static void io_loop(void)
{
  CORBA_Server_Environment _env = dice_default_environment;
  _env.user_data = (void *) &io_self;

  l4_io_server_loop(&_env);

  Panic("Left _infinite_ I/O server loop.");
}

/** do_args.
 * command line parameter handling
 */
static void do_args(int argc, char *argv[])
{
  signed char c;

  static int long_check;
  static int long_optind;
  static struct option long_options[] =
  {
    {"nolist", no_argument, &long_check, 2},
    {"include", required_argument, &long_check, 4},
    {"exclude", required_argument, &long_check, 5},
    {"events", no_argument, &long_check, 7},
    {"platform", required_argument, &long_check, 99},
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
        case 0:  /* long option */
          switch (long_check)
            {
            case 2:
              cfg_dev_list = 0;
              LOG_printf("Disabling listing of PCI devices.\n");
              break;
            case 4:
              if(add_device_inclusion(optarg))
                LOG_Error("invalid vendor:device string \"%s\"", optarg);
              break;
            case 5:
              if(add_device_exclusion(optarg))
                LOG_Error("invalid vendor:device string \"%s\"", optarg);
              break;
            case 7:
              cfg_events = 1;
              LOG_printf("Enabling events support.\n");
              break;
            default:
              /* ignore unknown */
              break;
            case 99:
#if defined(ARCH_arm)
              if (strcmp(optarg, "integrator") == 0)
                cfg_token = CFG_INTEGRATOR;
              else if (strcmp(optarg, "926") == 0)
                cfg_token = CFG_RV_EB_926;
              else if (strcmp(optarg, "mc") == 0)
                cfg_token = CFG_RV_EB_MC;
              else
                Panic("ARM platform \"%s\" not supported.", optarg);
              LOG_printf("Selected ARM platform is: %s\n", optarg);
#else
              LOG_printf("The current architecure does not support \"--platform\" parameter.\n");
#endif
              break;
            }
          break;
        default:
          /* ignore unknown */
          break;
        }
    }
}

/** Main of I/O server.
 *
 * main() function
 */
int main(int argc, char *argv[])
{
  int error;

  rmgr_init();

  do_args(argc, argv);

  /* init I/O info page */
  if ((error = io_info_init()))
    {
      LOGdL(DEBUG_ERRORS, "I/O info page initialization failed (%d)", error);
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
      LOGdL(DEBUG_ERRORS, "jiffies initialization failed (%d)", error);
      return error;
    }

  /* init submodules */
  if ((error = io_res_init(&io_self)))
    {
      LOGdL(DEBUG_ERRORS, "res initialization failed (%d)\n", error);
      return error;
    }

  if (!l4sigma0_kip_kernel_is_ux())
    {
      if ((error = io_static_cfg_init(&io_info, cfg_token)))
        {
          LOGdL(DEBUG_ERRORS, "static cfg initialization failed (%d)\n", error);
          return error;
        }

      if ((error = io_pci_init(cfg_dev_list)))
        {
          LOGdL(DEBUG_ERRORS, "pci initialization failed (%d)\n", error);
          return error;
        }
    }
  else
      /* init Fiasco's virtual H/W */
      if ((error = io_ux_init())) return error;

#ifndef ARCH_arm
  if ((error = OMEGA0_init()))
    {
      LOGdL(DEBUG_ERRORS, "omega0 initialization failed (%d)\n", error);
      return error;
    }
  io_info.omega0 = 1;
#endif

  /* we are up -> register at names */
  if (!names_register(IO_NAMES_STR))
    {
      LOGdL(DEBUG_ERRORS, "can't register at names");
      return -L4_ENOTFOUND;
    }

  if (cfg_events) init_events();

  /* go looping */
  io_loop();

  exit(0);
}
/** @} */
