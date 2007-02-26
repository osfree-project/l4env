/* $Id$ */
/**
 * \file	lxfuxlibc/server/fuxfprov/main.c
 * \brief	L4 file provider using native Linux syscalls under Fiasco/UX
 *
 * \date	08/18/2003
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author	Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 * This version is a modified version of loader/examples/fprov-l4/main.c
 *
 * */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>
#include <l4/generic_fprov/generic_fprov-server.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/env/env.h>
#include <l4/log/l4log.h>
#include <l4/lxfuxlibc/lxfuxlc.h>

#include <stdio.h>


typedef char l4_page_t[L4_PAGESIZE];

//static l4_threadid_t loader_id;
static l4_threadid_t dm_id;

/* placeholder for mapping a L4 page */
static l4_page_t io_buf __attribute__ ((aligned(L4_PAGESIZE)));
static l4_page_t map_page __attribute__ ((aligned(L4_PAGESIZE)));


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
l4fprov_file_open_component(CORBA_Object _dice_corba_obj,
    const char* fname,
    const l4_threadid_t *dm,
    l4_uint32_t flags,
    l4dm_dataspace_t *ds,
    l4_uint32_t *size,
    CORBA_Environment *_dice_corba_env)
{
  /*
   * I replaced the gz* calls from zlib though the normal calls (i.e. the
   * uncompressed ones) but left the double read, maybe someone can
   * reintroduce this later (i.e. build the zlib with this source or take
   * the one from l4env (hahahaha...))
   *                                                                Adam
   */

  int error;
  int fd;
  unsigned int offs;
  unsigned int fsize = 0, fsize_rounded;
  unsigned long fread;
  l4_addr_t fpage_addr;
  l4_size_t fpage_size;
  static char buf[64 << 10]; /* static -- we only have 16K stack... */
  const char *fname_name;

  /* touch memory to get it mapped in by Fiasco/UX so that
   * Linux can access it. */
  memset(buf, 0, sizeof(buf));

  /* if pathname is (nd)/tftpboot/user/ ignore leading path
   * and convert to /home/user/boot/
   * could be too OS specific!
   */ 
  if (strstr(fname, "(nd)/tftpboot/"))
    {
      char *p;
      printf("fuxfprov: Converting from %s to ", fname);

      fname += 14; /* fname is now at user/l4/path/prog */

      if (!(p = strchr(fname, '/'))) {
	printf("Invalid path starting with \"(nd)/tftpboot/\"!\n");
	return -L4_ENOTFOUND;
      }

      *p = 0;
      snprintf(buf, sizeof(buf), "/home/%s/boot/%s", fname, p + 1);
      *p = '/';
      fname = buf;
      printf("%s.\n", fname);
    }

  if (!(fname_name = strrchr(fname, '/')))
    fname_name = fname;
  else
    fname_name++;

  printf("fuxfprov: open \"%s\" by %x.%x\n", 
      fname, _dice_corba_obj->id.task, _dice_corba_obj->id.lthread);
      
  if ((fd = lx_open(fname, 0, 0)) == -1)
    {
      printf("Can't open \"%s\": -%d\n", fname, lx_errno);
      return -L4_ENOTFOUND;
    }
      
  /* Get the size of the _uncompressed_ file */
  while (1)
    {
      if ((fread = lx_read(fd, buf, sizeof(buf))) == -1)
	{
	  printf("Error decoding file %s: -%d\n", fname, lx_errno);
	  return -L4_EIO;
	}
      if (fread == 0)
	break;

      fsize += fread;
    }
  fsize_rounded = l4_round_page(fsize);

  lx_lseek(fd, 0, SEEK_SET);

  if ((error = l4dm_mem_open(dm_id, fsize_rounded, 0, 0, fname_name, 
			     (l4dm_dataspace_t *)ds)))
    {
      printf("Can't allocate dataspace with size %d (error %d)\n", 
	  (unsigned)fsize_rounded, error);
      lx_close(fd);
      return -L4_ENOMEM;
    }

  /* map in map area */
  memset(map_page, 0, sizeof(map_page));
  memset(io_buf, 0, sizeof(io_buf));

  /* map in dataspace page by page and copy file contents to it */
  for (offs=0; offs<fsize; offs+=L4_PAGESIZE)
    {
      if ((fread = lx_read(fd, io_buf, sizeof(io_buf))) == -1)
	{
	  printf("Error reading file \"%s\": -%d\n", fname, lx_errno);
	  l4dm_close((l4dm_dataspace_t *)ds);
	  lx_close(fd);
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
	  printf("Error %d requesting offset %08x "
		  "at ds_manager %x.%x\n", 
		  error, offs, 
		  ((l4dm_dataspace_t*)ds)->manager.id.task, 
		  ((l4dm_dataspace_t*)ds)->manager.id.lthread);
	  l4dm_close((l4dm_dataspace_t *)ds);
	  lx_close(fd);
	  return -L4_EINVAL;
	}

      /* copy file contents */
      memcpy(map_page, io_buf, L4_PAGESIZE);
    }

  lx_close(fd);

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

static void
server_loop(void)
{
  l4fprov_file_server_loop(NULL);
}

int
main(int argc, char **argv)
{
  l4_threadid_t me;
  const char *fname = strrchr(argv[0], '/');

  me = l4_myself();
  if (!fname)
    fname = argv[0];
  else
    fname++;
  
  if (argc > 1)
    {
      printf("L4 file provider\n"
	     "Usage:\n"
	     "  %s (no arguments)\n", 
	     fname);
      return -1;
    }

#if 0
  /* we need the loader */
  if (!names_waitfor_name("LOADER", &loader_id, 3000))
    {
      printf("LOADER not found\n");
      return 2;
    }
#endif

  /* we need a dataspace manager */
  dm_id = l4env_get_default_dsm();

  /* register ourself */
  if (!names_register("TFTP"))
    {
      printf("Failed to register TFTP\n");
      return -2;
    }

  printf("File provider started, registered as %x.%x\n",
         me.id.task, me.id.lthread);

  /* go into server mode */
  server_loop();

  return 0;
}

