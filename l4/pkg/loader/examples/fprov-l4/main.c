/* $Id$ */
/**
 * \file	loader/linux/fprov-l4/main.c
 * \brief	Linux file server serving L4 generic file provider requests
 *
 * \date	06/10/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 - 2009 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>
#include <stdlib.h>
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
#include <l4/generic_fprov/fprov_ext-server.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/env/env.h>
#include <l4/util/macros.h>
#include <l4/util/parse_cmd.h>

typedef char l4_page_t[L4_PAGESIZE];

static l4_threadid_t loader_id;
static l4_threadid_t dm_id;

/** Should we ignore path names and look for all files in the directory
    the server is started in? */
static int ignore_path;
static int enable_writing = 0; //fprov write support, default: disabled

/* placeholder for mapping a L4 page */
static l4_page_t io_buf __attribute__ ((aligned(L4_PAGESIZE)));
static l4_page_t map_page __attribute__ ((aligned(L4_PAGESIZE)));
static int       mapped = 1;

static void
free_map_area(void)
{
  if (mapped)
    {
      /* clear memory */
      l4_fpage_unmap(l4_fpage((l4_umword_t)map_page, L4_LOG2_PAGESIZE,
			      L4_FPAGE_RW, L4_FPAGE_MAP),
		     L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);
      mapped = 0;
    }
}

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

long 
l4fprov_file_ext_write_component (CORBA_Object _dice_corba_obj,
                              const char* fname /* in */,
                              const l4dm_dataspace_t *ds /* in */,
                              l4_size_t size /* in */,
                              CORBA_Server_Environment *_dice_corba_env)
{
  int error, fd, ret = 0;
  l4_offs_t offs;
  l4_size_t s = L4_PAGESIZE, w;

  if (!enable_writing)
    {
      printf("  writing to files are disabled! Ignore attempt of "
             l4util_idfmt" ...\n", l4util_idstr(*_dice_corba_obj));
      return -L4_ENOTSUPP;
    }

  // sanity checks
  if (fname == 0 || ds == 0 || size <= 0)
    return -L4_EINVAL;

  printf("  create & write file %s (%uB)\n", fname, size);

  // TODO fname check !
  fd = open(fname, O_CREAT | O_APPEND | O_RDWR);
  if (fd < 0)
    return -L4_EOPEN;

  // map in map area
  memset(map_page, 0, sizeof(map_page));
  memset(io_buf, 0, sizeof(io_buf));

  for (offs=0; (s == L4_PAGESIZE) && !ret; offs += L4_PAGESIZE)
    {
      // map page of dataspace
      s = (offs + L4_PAGESIZE > size) ? (size - offs) : L4_PAGESIZE;
      // map page of dataspace
      error = l4dm_map_ds(ds, offs, (l4_addr_t)map_page, L4_PAGESIZE, L4DM_RO);

      if (!error && mapped == 0)
        mapped = 1;
      if (error)
        {
          ret = -L4_ENOMAP;
          break;
        }

      // copy file contents
      memcpy(io_buf, map_page, s);

      w = write(fd, io_buf, s);
      if (w != s)
        ret = -L4_EIO;
    }

  // close file
  close(fd);

  // unmap ds area
  free_map_area();

  return ret;
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
	fname = (char*)(grub_fname+1);
    }

  if (!(fname_name = strrchr(fname, '/')))
    fname_name = fname;
  else
    fname_name++;

  if (ignore_path == 0)
    {
      printf("  open \"%s\" by " l4util_idfmt "\n",
          fname, l4util_idstr(*_dice_corba_obj));

      if ((fd = gzopen(fname, "r")) == NULL)
        {
          fprintf(stderr, "Can't open \"%s\"\n", fname);
          return -L4_ENOTFOUND;
        }
    }
  else
    {
      printf("  open \"%s\" by " l4util_idfmt "\n",
          fname_name, l4util_idstr(*_dice_corba_obj));

      if ((fd = gzopen(fname_name, "r")) == NULL)
        {
          fprintf(stderr, "Can't open \"%s\"\n", fname_name);
          return -L4_ENOTFOUND;
        }
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
      free_map_area();

      /* map page of dataspace */
      error = l4dm_map_pages(ds, offs, L4_PAGESIZE,
			     (l4_addr_t)map_page, L4_LOG2_PAGESIZE, 0, L4DM_RW,
			     &fpage_addr, &fpage_size);
      mapped = 1;
      if (error < 0)
	{
	  fprintf(stderr, "Error %d requesting offset %08x "
		  "at ds_manager " l4util_idfmt "\n",
		  error, offs, l4util_idstr(((l4dm_dataspace_t*)ds)->manager));
	  l4dm_close((l4dm_dataspace_t *)ds);
	  gzclose(fd);
	  return -L4_EINVAL;
	}

      /* copy file contents */
      memcpy(map_page, io_buf, L4_PAGESIZE);

      /* make sure that we don't have any mappings left since we transfer
       * the dataspace to the called at the end */
      free_map_area();
    }

  gzclose(fd);

  if ((error = l4dm_transfer((l4dm_dataspace_t*)ds, *_dice_corba_obj)))
    {
      printf("Error transfering dataspace ownership: %s (%d)\n",
	  l4env_errstr(error), error);
      l4dm_close((l4dm_dataspace_t*)ds);
      return -L4_EINVAL;
    }

  *size = fsize;

  return 0;
}

int
main(int argc, const char **argv)
{
  l4_threadid_t me;
  const char *fname = strrchr(argv[0], '/');
  struct stat buf;
  int error;

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

  if ((error = parse_cmdline(&argc, &argv,
                'n', "nopaths", "ignore paths in file names",
                PARSE_CMD_SWITCH, 1, &ignore_path,
                'w', "write", "enable fprov write support",
                PARSE_CMD_SWITCH, 1, &enable_writing,
                0)))
    {
      switch (error)
        {
        case -1: printf("Bad command line parameter\n"); break;
        case -2: printf("Out of memory in parse_cmdline()\n"); break;
        case -3: /* Unrecognized option */ break;
        case -4: /* Help requested */ break;
        default: printf("Error %d in parse_cmdline()", error); break;
        }
      return 1;
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

  printf("File provider started, registered as " l4util_idfmt "\n",
         l4util_idstr(me));

//  signal(SIGSEGV, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGINT,  signal_handler);

  /* go into server mode */
  l4fprov_file_ext_server_loop(NULL);
  return 0;
}
