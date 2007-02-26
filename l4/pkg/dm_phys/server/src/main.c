/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/main.c
 * \brief  Dataspace manager for phys. memory, main 
 *
 * \date   08/04/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* OSKIT/standard includes */
#include <stdlib.h>

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/util/getopt.h>
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>

/* DMphys/private includes */
#include <l4/dm_phys/consts.h>
#include "dm_phys-server.h" // server loop
#include "__config.h"
#include "__sigma0.h"
#include "__internal_alloc.h"
#include "__memmap.h"
#include "__pages.h"
#include "__dataspace.h"
#include "__dm_phys.h"
#include "__debug.h"
#include "__events.h"

/*****************************************************************************
 *** Global data
 *****************************************************************************/

/**
 * DMphys server thread id 
 */
l4_threadid_t dmphys_service_id = L4_INVALID_ID;

/**
 * Use rmgr
 */
static int use_rmgr = 0;

/**
 * Try to map 4M pages
 */
static int use_4M_pages = 1;

/**
 * Verbose startup
 */
static int  verbose = 0;

/**
 * Log tag
 */
char LOG_tag[9] = "DMphys";

/**
 * Starting the events thread
 */
static int using_events = 0;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Scan input string for a number
 * 
 * \param  input         Input string
 * \retval num           Number 
 * \retval nextp         Pointer in input string AFTER delimiter	
 *
 * \return 0 on success, -1 if something went wrong
 */
/*****************************************************************************/ 
static long
__get_num(char * input, long * num, char ** nextp)
{
  int    i;
  char * endp;

  i = 0;
  while ((input[i] != 0) && (input[i] != ',') && (input[i] != ' '))
    i++;

  if (i == 0)
    /* empty input string */
    return -1;

  if (input[i] != 0)
    {
      input[i] = 0;
      *nextp = &input[i + 1];
    }
  else
    *nextp= &input[i];

  *num = strtol(input,&endp,0);
  if (*endp != 0)
    {
      LOG_Error("DMphys: invalid number: \'%s\'!", input);
      return -1;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Set memory pool configuration
 * 
 * \param  pool          Pool number
 * \param  str           Argument string
 *
 * The argument string format is 'size[,low,high[,name]]'
 */
/*****************************************************************************/ 
static void
__pool_config(int pool, char * str)
{
  char *    next;
  char *    name;
  l4_size_t size;
  l4_addr_t low,high;
  long      num;
 
  if (str == NULL)
    {
      LOG_Error("DMphys: invalid memory area description for pool %d", pool);
      return;
    }

  LOGdL(DEBUG_ARGS, "pool %d = %s:", pool, str);

  /* pool size */
  if (__get_num(str,&num,&next) < 0)
    {
      LOG_Error("DMphys: invalid size for pool %d: %s!", pool, str);
      return;
    }
  size = num;

  /* memory range low address */
  str = next;
  if (__get_num(str, &num, &next) == 0)
    {
      /* memory range high address */
      low = num;
      str = next;
      if (__get_num(str, &num, &next) < 0)
	{
	  LOG_Error("DMphys: invalid memory range for pool %d!", pool);
	  return;
	}

      high = num;
      /* pool name */
      if (strlen(next) > 0)
	name = next;
      else
	name = NULL;
    }
  else
    {
      low = 0;
      high = 0;
      name = NULL;
    }

#if DEBUG_ARGS
  if (name != NULL)
    LOG_printf(" size 0x%08x, low 0x%08lx, high 0x%08lx, name \'%s\'\n",
           size, low, high, name);
  else
    LOG_printf(" size 0x%08x, low 0x%08lx, high 0x%08lx\n", size, low, high);
#endif

  /* set pool memory area */
  dmphys_memmap_set_pool_config(pool, size, low, high, name);
}

/*****************************************************************************/
/**
 * \brief  Reserve memory area
 * 
 * \param  str           Argument string
 *
 * The argument string format is 'low,high'
 */
/*****************************************************************************/ 
static void
__reserve(char * str)
{
  char *    nextp;
  l4_addr_t low,high;
  long      num;

  /* low address */
  if (__get_num(str, &num, &nextp) < 0)
    {
      LOG_Error("DMphys: invalid low address for reserve!");
      return;
    }
  low = num;

  /* high address */
  str = nextp;
  if (__get_num(str, &num, &nextp) < 0)
    {
      LOG_Error("DMphys: invalid high address for reserve!");
      return;
    }
  high = num;

  if (low >= high)
    {
      LOG_Error("DMphys: invalid memory range for reserve (0x%08lx-0x%08lx)!",
                low, high);
      return;
    }

  LOGdL(DEBUG_ARGS, "reserve 0x%08lx-0x%08lx", low, high);

  /* reserve */
  if (dmphys_memmap_reserve(low, high - low) < 0)
    LOG_Error("DMphys: reserve memory area 0x%08lx-0x%08lx failed!", low, high);
    
  /* done */
}

/*****************************************************************************/
/**
 * \brief  Parse command line options.
 * 
 * \param  argc          Number of command line arguments
 * \param  argv          Argument list
 * 
 * DMphys suuports following arguments:
 */
/*****************************************************************************/ 
static void
__parse_command_line(int argc, char *argv[])
{
  char      c;
  long      num;
  char *    nextp;

  static struct option long_options[] =
    {
      {"pool",        1, 0, 'p'},
      {"mem",         1, 0, 'm'},
      {"isa",         1, 0, 'i'},
      {"reserve",     1, 0, 'r'},
      {"low",         1, 0, 'l'},
      {"high",        1, 0, 'h'},
      {"rmgr",        0, 0, 'R'},
      {"no_4M_pages", 0, 0, 'n'},
      {"verbose",     0, 0, 'v'},
      {"events",      0, 0, 'e'},
      {0, 0, 0, 0}
    };

  /* read command line arguments */
  while (1)
    {
      c = getopt_long(argc, argv, "p:m:i:r:l:h:Rnve", long_options, NULL);
      
      if (c == (char) -1)
	break;

#if DEBUG_ARGS
      if (optarg)
	LOGL("-%c=%s", c, optarg);
      else
	LOGL("-%c", c);
#endif

      switch(c)
	{
	case 'p':
	  /* memory pool configuration */
	  if (__get_num(optarg, &num, &nextp) < 0)
	    LOG_Error("DMphys: invalid pool number!");
	  else
	    __pool_config(num, nextp);
	  break;

	case 'm':
	  /* default memory pool */
	  __pool_config(DMPHYS_MEM_DEFAULT_POOL, optarg);
	  break;
	  
	case 'i':
	  /* ISA DMA memory pool */
	  __pool_config(DMPHYS_MEM_ISA_DMA_POOL, optarg);
	  break;

	case 'r':
	  /* reserve memory area */
	  __reserve(optarg);
	  break;

	case 'l':
	  /* set memory map search low address */
	  if (__get_num(optarg, &num, &nextp) < 0)
	    LOG_Error("DMphys: invalid low address!");
	  else
	    {
	      LOGdL(DEBUG_ARGS, "low address 0x%08lx", num);
	      dmphys_memmap_set_mem_low(num);
	    }
	  break;

	case 'h':
	  /* set memory map search high address */
	  if (__get_num(optarg, &num, &nextp) < 0)
	    LOG_Error("DMphys: invalid high address!");
	  else
	    {
	      LOGdL(DEBUG_ARGS, "high address 0x%08lx", num);
	      dmphys_memmap_set_mem_high(num);
	    }
	  break;
	  
	case 'R':
	  /* use rmgr to allocate page areas */
	  LOGdL(DEBUG_ARGS,"use rmgr");
	  use_rmgr = 1;
	  break;

	case 'n':
	  /* don't use 4M pages */
	  LOGdL(DEBUG_ARGS, "no 4M pages");
	  use_4M_pages = 0;
	  break;

	case 'v':
	  /* verbose startup */
	  LOGdL(DEBUG_ARGS, "verbose");
	  verbose = 1;
	  break;
	  
	case 'e':
          /* listen to event server */
	  LOGdL(DEBUG_ARGS, "events");
	  using_events = 1;
	  break;

	default:
	  /* invalid option */
	  LOG_Error("DMphys: invalid option \'%c\'!", c);
	}
    }
}

/*****************************************************************************/
/**
 * \brief  Initialize DMphys
 *	
 * \return 0 on success, -1 if something went wrong.
 */
/*****************************************************************************/ 
static int
__init(int argc, char **argv)
{
  /* Sigma0 communication */
  if (dmphys_sigma0_init() < 0)
    return -1;

  /* initialize internal memory allocation */
  if (dmphys_internal_alloc_init() < 0)
    return -1;

  /* initialize low level memory map handling */
  if (dmphys_memmap_init() < 0)
    return -1;

  /* reserve pages used by internal heap in memory map */
  dmphys_internal_alloc_init_reserve();

  /* initialize page pool handling */
  if (dmphys_pages_init() < 0)
    return -1;

  /* parse command line */
  __parse_command_line(argc, argv);

  /* setup page pools */
  if (dmphys_memmap_setup_pools(use_rmgr, use_4M_pages) < 0)
    return -1;
  
  /* initialize internal dataspace descriptor handling */
  if (dmphys_ds_init() < 0)
    return -1;

  if (verbose)
    {
      /* show DMphys information */
      dmphys_memmap_show();
      LOG_printf("\n");
      dmphys_pages_dump_used_pools();
    }

  /* finished with setup, refill internal memory pool */
  dmphys_internal_alloc_update();

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Main function.
 */
/*****************************************************************************/ 
int
main(int argc, char **argv)
{
  /* setup DMphys */
  if (__init(argc,argv) < 0)
    {
      Panic("DMphys: Initialization failed!");
      return -1;
    }

  /* set service thread id */
  dmphys_service_id = l4_myself();

  /* register at nameserver */
  if (!names_register(L4DM_MEMPHYS_NAME))
    {
      Panic("DMphys: can't register at nameserver");
      return -1;
    }

  /* start thread waiting for exit events */
  if (using_events)
    init_events();

  /* start server loop */
  if_l4dm_memphys_server_loop(NULL);

  /* this should never happen! */
  Panic("DMphys: left server loop!");

  /* done */
  return 0;
}
