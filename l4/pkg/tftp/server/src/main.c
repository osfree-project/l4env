/* $Id$ */
/**
 * \file	tftp/server/src/main.c
 *
 * \date	17/08/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * Simple DROPS TFTP server.
 * Network code adapted from GRUB */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "netboot/etherboot.h"
#include "netboot/netboot.h"

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/consts.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/generic_fprov/generic_fprov-server.h>
#include <l4/thread/thread.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/getopt.h>
#include <l4/util/macros.h>
#include <l4/util/util.h>

// #define DEBUG_LOAD
// #define DEBUG_REQUEST

/* TFTP server address */
static in_addr tftp_server_addr;
static int have_tftp_server_addr = 0;
static char real_main_stack[2*L4_PAGESIZE];

extern int disp_filesizebarrier;
extern int disp_filesize;

extern char tftp_orename[16];

/**
 * Parse command line
 *
 * \param  argc          Number of command line arguments
 * \param  argv          Command line arguments
 */
static void
__parse_command_line(int argc, char * argv[])
{
  char c;
  static struct option long_options[] =
  {
    {"server", 1, 0, 's'},
    {"orename", 1, 0, 'o'},
    {0, 0, 0, 0}
  };

  /* read command line arguments */
  while (1)
    {
      c = getopt_long(argc, argv, "s:i", long_options, NULL);
      if (c == -1)
	break;

      switch (c)
	{
	case 's':
	  /* TFTP server address */
	  if (optarg == NULL)
	    {
	      printf("No TFTP server address!\n");
	      break;
	    }

	  if (!inet_aton(optarg, &tftp_server_addr))
	    printf("Invalid TFTP server address: %s!\n",optarg);
	  else
	    {
	      have_tftp_server_addr = 1;
	      printf("Using TFTP server %s (0x%08x)\n",optarg,
		     tftp_server_addr.s_addr);
	    }

	  /* done */
	  break;

        case 'o':
          LOG("orename: %s", optarg);
          strncpy(tftp_orename, optarg, sizeof(tftp_orename));
          tftp_orename[sizeof(tftp_orename)-1] = 0;
          break;

	default:
	  printf("Invalid argument: %c\n",c);
	}
    }
}

/**
 * Return a new dataspace including the image of a L4 module
 *
 * \param request	pointer to Flick request structure
 * \param fname		requested module filename
 * \param dm		dataspace manager for allocating the image dataspace
 * \param flags		flags used for creating a dataspace
 * \retval ds		dataspace including the file image
 * \retval _ev		Flick exception structure (unused)
 * \return		0 on success
 *			-L4_ENOMEM if allocation failed */
long
l4fprov_file_open_component (CORBA_Object _dice_corba_obj,
                             const char* fname,
                             const l4_threadid_t *dm,
                             unsigned long flags,
                             l4dm_dataspace_t *ds,
                             l4_size_t *size,
                             CORBA_Server_Environment *_dice_corba_env)
{
  int read_size;
  int error;
  unsigned char *addr;
  char buf[L4DM_DS_NAME_MAX_LEN];
  const char *ptr;

  if (!netboot_open((char*)fname))
    {
      /* file not found */
      printf("File not found: \"%s\"\n", fname);
      return -L4_ENOTFOUND;
    }

  /* after succesful open of file, netboot_filemax consists of
   * the length of the file */
  *size = netboot_filemax;

  ptr = strrchr(fname, '/');
  if (!ptr)
    ptr=fname;
  else
    ptr++;
  snprintf(buf, L4DM_DS_NAME_MAX_LEN, "tftp image: %s", ptr);

  if (!(addr = l4dm_mem_ds_allocate_named_dsm(*size, flags, buf, *dm,
					  (l4dm_dataspace_t *)ds)))
    {
      printf("Allocating dataspace of size %d failed\n", *size);
      netboot_close();
      return -L4_ENOMEM;
    }

  printf("Loading %s [%dkB]\n", fname, (*size + 1023) / 1024);

  /* Reset display file position */
  disp_filesizebarrier = disp_filesize = 0;

  read_size = netboot_read(addr, *size);

  netboot_close();

  /* file image is not mapped to our address space anymore */
  if ((error = l4rm_detach(addr)))
    {
      printf("Error %d detaching dataspace\n", error);
      l4dm_close((l4dm_dataspace_t*)ds);
      return -L4_ENOMEM;
    }

  if (!read_size)
    {
      printf("Error reading file %s\n"
	     "TFTP error was %d (%s)\n",
	      fname,
	      netboot_errnum, err_list[netboot_errnum]);
      l4dm_close((l4dm_dataspace_t*)ds);
      return -L4_EIO;
    }

  /* set dataspace owner to client */
  if ((error = l4dm_transfer((l4dm_dataspace_t *)ds,*_dice_corba_obj)))
    {
      printf("Error transfering dataspace ownership: %s (%d)\n",
	     l4env_errstr(error), error);
      l4dm_close((l4dm_dataspace_t*)ds);
      return -L4_EINVAL;
    }

  /* done */
  return 0;
}

static void
server_loop(void)
{
  l4fprov_file_server_loop(NULL);
}

/* The only reason to start a new thread here is that we make sure that
 * out stack is paged by Sigma0/Rmgr and therefore is mapped one-by-one.
 * Etherboot requires that (network packets as local parameters...) */
static void
real_main (void *dummy)
{
  l4_addr_t netbuf, gunzipbuf;

  /* get dataspace for the net buffer */
  netbuf = (l4_addr_t)l4dm_mem_allocate_named(NETBUF_SIZE,0,"tftp netbuff");
  if (netbuf == 0)
    {
      printf("Allocate netbuff failed\n");
      exit(-1);
    }

  /* get dataspace for gunzip allocator buffer */
  gunzipbuf = (l4_addr_t)l4dm_mem_allocate_named(GUNZIP_SIZE,0,"tftp gunzip");
  if (gunzipbuf == 0)
    {
      printf("Allocate gunzip buffer failed\n");
      exit(-1);
    }

  if (netboot_init(netbuf, gunzipbuf))
    {
      printf("Can't determine network card\n");
      exit(-1);
    }

  if (!names_register("TFTP"))
    {
      printf("failed to register at name server\n");
      exit(-1);
    }

  if (have_tftp_server_addr)
    netboot_set_server(tftp_server_addr);

  server_loop();
}


int
main (int argc, char **argv)
{
  int ret;

  __parse_command_line(argc, argv);

  netboot_show_drivers();

  if ((ret = l4thread_create_long(L4THREAD_INVALID_ID, real_main,
				  ".real_main",
				  (unsigned)real_main_stack
				    + sizeof(real_main_stack),
				  sizeof(real_main_stack),
				  L4THREAD_DEFAULT_PRIO, 0,
				  L4THREAD_CREATE_ASYNC)) < 0)
    {
      Panic ("Error %d creating real_main thread", ret);
    }

  for (;;)
    l4thread_sleep_forever();
}

