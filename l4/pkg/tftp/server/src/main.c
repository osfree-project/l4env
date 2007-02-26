/* $Id$ */
/**
 * \file	tftp/server/src/main.c
 *
 * \date	17/08/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * 
 * Simple DROPS TFTP server. 
 * Network code adapted from GRUB */

/* local includes */
#include "netboot/netboot.h"
#include "netboot/etherboot.h"

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/consts.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/generic_fprov/generic_fprov-server.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/getopt.h>
#include <l4/generic_io/libio.h>

#include <stdio.h>
#include <string.h>

// #define DEBUG_LOAD
// #define DEBUG_REQUEST

/* TFTP server address */
static in_addr tftp_server_addr;
static int have_tftp_server_addr = 0;

int use_l4io = 0;	/* whether to use L4IO server or not, default no */

extern int disp_filesizebarrier;
extern int disp_filesize;

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
    {"l4io", 0, 0, 'i'},
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
	      printf("Using TFTP server %s (0x%08lx)\n",optarg,
		     tftp_server_addr.s_addr);
	    }

	  /* done */
	  break;

	case 'i':
	  {
	    l4io_info_t *io_info_addr = (l4io_info_t*)-1;

	    if (l4io_init(&io_info_addr, L4IO_DRV_INVALID))
	      {
		enter_kdebug("Couldn't connect to L4 IO server!");

		/* Just go on without the IO server */
	      }
	    else
	      use_l4io = 1;
	  }
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
 * \return 		0 on success
 * 			-L4_ENOMEM if allocation failed */
l4_int32_t 
l4fprov_file_server_open(sm_request_t *request, const char *fname,
			 const l4fprov_threadid_t *dm, l4_uint32_t flags,
			 l4fprov_dataspace_t *ds, l4_uint32_t *size,
			 sm_exc_t *_ev)
{
  int read_size;
  int error;
  char *addr;
  char buf[L4DM_DS_NAME_MAX_LEN];
  const char *ptr;

  if (!netboot_open((char*)fname))
    {
      /* file not found */
      printf("File not found: %s\n", fname);
      return -L4_ENOTFOUND;
    }

  /* after succesful open of file, netboot_filemax consists of
   * the length of the file */
  *size = netboot_filemax;

  ptr = strrchr(fname, '/');
  if(!ptr) ptr=fname;
    else ptr++;
  snprintf(buf, L4DM_DS_NAME_MAX_LEN-1, "tftp image: %s", ptr);
  buf[L4DM_DS_NAME_MAX_LEN-1]=0;
  	   

  if (!(addr = l4dm_mem_ds_allocate_named(*size, flags, buf,
					  (l4dm_dataspace_t *)ds)))
    {
      printf("Allocating dataspace of size %d failed\n", *size);
      netboot_close();
      return -L4_ENOMEM;
    }

  printf("Loading %s [%dkB]\n", fname, (*size + 1023) / 1024);
  
  /* Reset display file position */
  disp_filesizebarrier = disp_filesize = 0;
  
  read_size = netboot_read((char*) addr, *size);

  netboot_close();

  /* file image is not mapped to our address space anymore */
  if ((error = l4rm_detach((void *)addr)))
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
  if ((error = l4dm_transfer((l4dm_dataspace_t *)ds,request->client_tid)))
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
  int ret;
  sm_request_t request;
  l4_ipc_buffer_t ipc_buf;
  l4_msgdope_t result;

  /* enter request loop */
  flick_init_request(&request, &ipc_buf);
  for (;;)
    {
      result = flick_server_wait(&request);
      while (!L4_IPC_IS_ERROR(result))
	{
#ifdef DEBUG_REQUEST
	  LOGL("request 0x%08x, src %x.%x\n", ipc_buf.buffer[0],
	      request.client_tid.id.task, request.client_tid.id.lthread);
#endif
	  switch (ret = l4fprov_file_server(&request))
	    {
	    case DISPATCH_ACK_SEND:
	      result = flick_server_reply_and_wait(&request);
	      break;

	    default:
	      LOGL("Flick dispatch error (%d)!\n", ret);
	      result = flick_server_wait(&request);
	      break;
	    }
	}
      printf("Flick IPC error (0x%08x)!\n", result.msgdope);
    }
}

int
main (int argc, char **argv)
{
  l4_addr_t netbuf, gunzipbuf;

  LOG_init("tftp");

  /* parse command line */
  __parse_command_line(argc, argv);

  /* get dataspace for the net buffer */
  netbuf = (l4_addr_t)l4dm_mem_allocate_named(NETBUF_SIZE,0,"tftp netbuff");
  if (netbuf == 0)
    {
      printf("Allocate netbuff failed\n");
      return -1;
    }

  /* get dataspace for gunzip allocator buffer */
  gunzipbuf = (l4_addr_t)l4dm_mem_allocate_named(GUNZIP_SIZE,0,"tftp gunzip");
  if (gunzipbuf == 0)
    {
      printf("Allocate gunzip buffer failed\n");
      return -1;
    }

  if (netboot_init(netbuf, gunzipbuf))
    {
      printf("Can't determine network card\n");
      return -1;
    }

  if (!names_register("TFTP"))
    {
      printf("failed to register TFTP\n");
      return -1;
    }
  
  if (have_tftp_server_addr)
    netboot_set_server(tftp_server_addr);

  server_loop();
  return 0;
}

