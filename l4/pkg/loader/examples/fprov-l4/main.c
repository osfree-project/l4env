/* $Id$ */
/**
 * \file	loader/linux/fprov-l4/main.c
 *
 * \date	06/10/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief	Linux file server serving L4 generic file provider requests */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <zlib.h>

#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>
#include <l4/generic_fprov/generic_fprov-server.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/env/env.h>

typedef char l4_page_t[L4_PAGESIZE];

static l4_threadid_t loader_id;
static l4_threadid_t dm_id;

/* placeholder for mapping a L4 page */
static l4_page_t io_buf __attribute__ ((aligned(L4_PAGESIZE)));
static l4_page_t map_page __attribute__ ((aligned(L4_PAGESIZE)));

static void
signal_handler(int code)
{
  names_unregister("FPROV-L4");
  printf("File provider aborted (signal %d), unregistered\n", code);
  exit(-2);
}

static void __attribute__((unused))
linux_enter_kdebug(void)
{
#define L4IOTYPE (0x34)
#define L4KDEBUG  _IO(L4IOTYPE, 2)
  int fn;

  if ((fn = open("/proc/l4/ctl", O_RDWR)) < 0)
    {
      printf("linux_enter_kdebug: unable to open \"/proc/l4/ctl\"\n");
      return;
    }
  if (ioctl(fn, L4KDEBUG, 0))
    {
      printf("linux_enter_kdebug: "
	      "unable to execute ioctl on \"proc/l4/ctl\"\n");
      return;
    }
  close(fn);
}

/**
 * Return a new dataspace including the image of a L4 module
 * 
 * \param request	pointer to Flick request structure
 * \param fname		requested module filename
 * \param dm		dataspace manager for allocating the image dataspace
 * \param flags		flags (unused)
 * \retval ds		dataspace including the file image
 * \retval size		size of file
 * \retval _ev		Flick exception structure (unused)
 * \return 		0 on success
 * 			-L4_ENOMEM if allocation failed */
l4_int32_t 
l4fprov_file_server_open(sm_request_t *request, const char *fname,
			 const l4fprov_threadid_t *dm, l4_uint32_t flags,
			 l4fprov_dataspace_t *ds, l4_uint32_t *size,
			 sm_exc_t *_ev)
{
  int error;
  gzFile fd;
  unsigned int offs;
  off_t fsize = 0, fsize_rounded;
  ssize_t fread;
  l4_addr_t fpage_addr;
  l4_size_t fpage_size;
  char buf[64 << 10];
  const char *fname_name;
  const char *grub_path, *grub_fname;

  /* if pathname is (nd)/... ignore leading path */
  if ((grub_path = strstr(fname, "(nd)/tftpboot/")))
    {
      if ((grub_fname = strchr(grub_path + 14, '/')))
	fname = grub_fname+1;
    }

  if (!(fname_name = strrchr(fname, '/')))
    fname_name = fname;
  else
    fname_name++;

  printf("  open \"%s\" by %x.%x\n", 
      fname, request->client_tid.id.task, request->client_tid.id.lthread);
      
  if ((fd = gzopen(fname, "r")) == NULL)
    {
      fprintf(stderr, "Can't open \"%s\"\n", fname);
      return -L4_ENOTFOUND;
    }
      
  /* Get the size of the _uncompressed_ file */
  while (1)
    {
      if ((fread = gzread(fd, buf, sizeof(buf))) < 0)
	{
	  fprintf(stderr, "Error decoding file %s\n", fname);
	  return -L4_EIO;
	}
      if (fread == 0)
	break;

      fsize += fread;
    }
  fsize_rounded = l4_round_page(fsize);

  gzseek(fd, 0, SEEK_SET);

  if ((error = l4dm_mem_open(dm_id, fsize_rounded, 0, 0, fname_name, 
			     (l4dm_dataspace_t *)ds)))
    {
      fprintf(stderr, "Can't allocate dataspace with size %d (error %d)\n", 
	  (unsigned)fsize_rounded, error);
      gzclose(fd);
      return -L4_ENOMEM;
    }

  /* map in map area */
  memset(map_page, 0, sizeof(map_page));
  memset(io_buf, 0, sizeof(io_buf));

  /* map in dataspace page by page and copy file contents to it */
  for (offs=0; offs<fsize; offs+=L4_PAGESIZE)
    {
      if ((fread = gzread(fd, io_buf, sizeof(io_buf))) == -1)
	{
	  fprintf(stderr, "Error reading file \"%s\"\n", fname);
	  l4dm_close((l4dm_dataspace_t *)ds);
	  gzclose(fd);
	  return -L4_EIO;
	}

      if (fread < sizeof(io_buf))
	{
	  /* clear rest of page */
	  memset(io_buf+fread, 0, sizeof(io_buf)-fread);
	}

      /* clear memory */
      l4_fpage_unmap(l4_fpage((l4_umword_t)map_page, L4_LOG2_PAGESIZE,
			       L4_FPAGE_RW, L4_FPAGE_MAP),
		     L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);
  
      /* map page of dataspace */
      error = l4dm_map_pages((l4dm_dataspace_t *)ds,offs,L4_PAGESIZE,
			     (l4_addr_t)map_page,L4_LOG2_PAGESIZE,0,L4DM_RW,
			     &fpage_addr,&fpage_size);
      if (error < 0)
	{
	  fprintf(stderr, "Error %d requesting offset %08x "
		  "at ds_manager %x.%x\n", 
		  error, offs, 
		  ((l4dm_dataspace_t*)ds)->manager.id.task, 
		  ((l4dm_dataspace_t*)ds)->manager.id.lthread);
	  l4dm_close((l4dm_dataspace_t *)ds);
	  gzclose(fd);
	  return -L4_EINVAL;
	}

      /* copy file contents */
      memcpy(map_page, io_buf, L4_PAGESIZE);
    }

  gzclose(fd);

  if ((error = l4dm_transfer((l4dm_dataspace_t*)ds, request->client_tid)))
    {
      printf("Error transfering dataspace ownership: %s (%d)\n",
	  l4env_errstr(error), error);
      l4dm_close((l4dm_dataspace_t*)ds);
      return -L4_EINVAL;
    }
  
  *size = fsize;

  return 0;
}

static void
server_loop(void)
{
  int ret;
  sm_request_t request;
  l4_ipc_buffer_t ipc_buf;
  l4_msgdope_t result;

  flick_init_request(&request, &ipc_buf);
  for (;;)
    {
      result = flick_server_wait(&request);
      while (!L4_IPC_IS_ERROR(result))
	{
#if DEBUG_REQUEST
	  fprintf(stderr, "request 0x%08x, src %x.%x\n", ipc_buf.buffer[0],
		  request.client_tid.id.task, request.client_tid.id.lthread);
#endif
	  switch (ret = l4fprov_file_server(&request))
	    {
	    case DISPATCH_ACK_SEND:
	      result = flick_server_reply_and_wait(&request);
	      break;

	    default:
	      fprintf(stderr, "Flick dispatch error (%d)!\n", ret);
	      result = flick_server_wait(&request);
	      break;
	    }
	}
      fprintf(stderr, "Flick IPC error (0x%08x)!\n", result.msgdope);
    }
}

int
main(int argc, char **argv)
{
  l4_threadid_t me;
  const char *fname = strrchr(argv[0], '/');
  struct stat buf;

  if (stat("/proc/l4", &buf))
    {
      fprintf(stderr, "This binary requires L4Linux!\n");
      exit(-1);
    }

  me = l4_myself();
  if (!fname)
    fname = argv[0];
  else
    fname++;
  
  if (argc > 1)
    {
      fprintf(stderr, "L4 file provider\n"
	              "Usage:\n"
	              "  %s (no arguments)\n", 
		      fname);
      return -1;
    }

  /* we need the loader */
  if (!names_waitfor_name("LOADER", &loader_id, 3000))
    {
      fprintf(stderr, "LOADER not found\n");
      return 2;
    }

  /* we need a dataspace manager */
  dm_id = l4env_get_default_dsm();

  /* register ourself */
  if (!names_register("FPROV-L4"))
    {
      fprintf(stderr, "Failed to register FPROV-L4\n");
      return -2;
    }

  printf("File provider started, registered as %x.%x\n",
      me.id.task, me.id.lthread);

//  signal(SIGSEGV, signal_handler);
  signal(SIGKILL, signal_handler);
  signal(SIGINT,  signal_handler);

  /* go into server mode */
  server_loop();

  return 0;
}

