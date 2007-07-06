/**
 * \file   ldso/lib/ldso/emul_linux.c
 * \brief  Adaption layer for Linux system calls to L4env
 *
 * \date   2005/05/12
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>
#include <stdlib.h>
#include <l4/sys/consts.h>
#include <l4/sys/kdebug.h>
#include <l4/env/env.h>
#include <l4/log/l4log.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/generic_fprov/generic_fprov-client.h>

#include "dl-syscall.h"
#include "infopage.h"
#include "emul_linux.h"
#include "elf.h"

#define MMAP_ENTRIES	64

#if DEBUG_LEVEL>1
#define INFO		1
#else
#define INFO		0
#endif
#if DEBUG_LEVEL>0
#define DBG		1
#else
#define DBG		0
#endif

/* We maintain a simple double-linked list for mmap() regions. */
typedef struct mmap_region_t
{
  l4_addr_t		addr;
  l4_size_t		size;
  l4dm_dataspace_t	ds;
  struct mmap_region_t	*prev;
  struct mmap_region_t	*next;
#if DEBUG_LEVEL>0
  char                  name[32];
  char                  unseen;
#endif
} mmap_region_t;

static mmap_region_t	mmap_regions[MMAP_ENTRIES];
static mmap_region_t	*mmap_region_list;

/* We maintain no list here since it seems that only one file is open
 * at one time. */
static int		open_fd;	/**< current opened file descriptor */
static int		open_fd_magic;	/**< file descriptor ID */
static l4dm_dataspace_t	open_ds;	/**< current opened image dataspace */
static l4_addr_t	open_addr;	/**< current opened image address */
static l4_size_t	open_size;	/**< current opened image size */
static l4_addr_t	open_offs;	/**< seek position in current file */
#if DEBUG_LEVEL>0
static char             open_fname[32];	/**< name of the current open file */
#endif

/** Find the according mmap_region_t to <addr, addr+size>. */
static mmap_region_t*
mmap_find_region(l4_addr_t addr, l4_size_t size)
{
  mmap_region_t *m;

  for (m=mmap_region_list; m; m=m->next)
    {
      if (m->addr <= addr && m->addr+m->size >= addr+size)
	return m;
    }

  return 0;
}

/** Find a free region with a defined size. */
static l4_addr_t
mmap_find_free_addr(l4_size_t size, mmap_region_t **prev)
{
  mmap_region_t *m = mmap_region_list;
  l4_addr_t a = MMAP_START;

  *prev = 0;
  while (m)
    {
      if ((m->addr - a) >= size)
	return a;
      *prev = m;
      a     = m->addr+m->size;
      m     = m->next;
    }

  return ((MMAP_END-a) >= size) ? a : 0;
}

/** Check if region at specified address is free. */
static l4_addr_t
mmap_test_free_addr(l4_addr_t addr, l4_size_t size, mmap_region_t **prev)
{
  mmap_region_t *m = mmap_region_list;

  *prev = 0;
  while (m)
    {
      if (m != mmap_region_list && m->addr+m->size > addr)
	return 0;

      if (m->next)
	{
	  if (addr+size <= m->next->addr)
	    break;
	}
      *prev = m;
      m = m->next;
    }

  return addr;
}

/** Find an unused mmap_region_t entry. */
static mmap_region_t*
mmap_find_free_slot(mmap_region_t *start)
{
  mmap_region_t *m = start ? start+1 : mmap_regions;
  mmap_region_t *e = start ? start   : mmap_regions+MMAP_ENTRIES;

  for (; m!=e; m++)
    {
      if (m>=mmap_regions+MMAP_ENTRIES)
	m = mmap_regions;
      if (!m->size)
	{
#if DEBUG_LEVEL>0
	  m->unseen = 1;
#endif
	  return m;
	}
    }

  return 0;
}

/** Insert mmap region into the double linked list. */
static void
mmap_insert_list(mmap_region_t *m, mmap_region_t *prev)
{
  m->prev = prev;
  if (prev)
    {
      m->next = prev->next;
      m->prev->next = m;
    }
  else
    {
      m->next = mmap_region_list;
      mmap_region_list = m;
    }
  if (m->next)
    m->next->prev = m;
}

/** Delete mmap region from the double linked list. */
static void
mmap_delete_list(mmap_region_t *m)
{
  if (m->prev)
    m->prev->next = m->next;
  if (m->next)
    m->next->prev = m->prev;
  if (mmap_region_list == m)
    mmap_region_list = m->next;
}

/** Show all mmap regions (for debugging purposes). */
void
_dl_mmap_list_regions(int only_unseen)
{
#if DEBUG_LEVEL>0
  mmap_region_t *m;

  if (!mmap_region_list)
    {
      printf("  <region list empty>\n");
      return;
    }

  for (m=mmap_region_list; m; m=m->next)
    if (!only_unseen || m->unseen)
      {
	printf("  "l4_addr_fmt" - "l4_addr_fmt" ds %4d (%4zdKB) %s\n", 
	    m->addr, m->addr+m->size, m->ds.id, m->size/1024, m->name);
	m->unseen = 0;
      }
#endif
}

/** Map anonymous memory at a free place. */
static void*
mmap_anonymous(l4_addr_t want_addr, l4_size_t size, int prot, 
	       l4_addr_t *phys, const char *name)
{
  int error;
  l4_addr_t addr;
  mmap_region_t *prev, *m;

  size = l4_round_page(size);
  if (want_addr == ~0U)
    {
      if (!(addr = mmap_find_free_addr(size, &prev)))
	{
	  LOGd(DBG, "No free mmap address size "l4_addr_fmt, (l4_addr_t)size);
	  return 0;
	}
    }
  else
    {
      if (!(addr = mmap_test_free_addr(want_addr, size, &prev)))
	{
	  LOGd(DBG, "Address at "l4_addr_fmt" size "l4_addr_fmt" not free", 
	      want_addr, (l4_addr_t)size);
	  return 0;
	}
    }
  if (!(m = mmap_find_free_slot(prev)))
    {
      LOGd(DBG, "No free mmap slot (incease MMAP_ENTRIES!)");
      return 0;
    }
  if ((error = l4dm_mem_open(global_env->memserv_id, size,
			     0, L4DM_PINNED | (phys ? L4DM_CONTIGUOUS : 0),
			     "anon memory", &m->ds)))
    {
      LOGd(DBG, "Error %d in mmap_open", error);
      return 0;
    }
  if (phys)
    {
      l4_addr_t addr;
      l4_size_t psize;
      if ((error = l4dm_mem_ds_phys_addr(&m->ds, 0, L4DM_WHOLE_DS, 
					 &addr, &psize)))
	{
	  LOGd(DBG, "Error %d requesting physical addr of ds %d",
	         error, m->ds.id);
	  l4dm_close(&m->ds);
	  return 0;
	}
      *phys = addr;
    }
  if ((error = l4dm_map_ds(&m->ds, 0, addr, size, L4DM_RW)))
    {
      LOGd(DBG, "Error %d in mmap_map", error);
      l4dm_close(&m->ds);
      return 0;
    }
  m->addr = addr;
  m->size = size;
#if DEBUG_LEVEL>0
    {
      int i;
      for (i=0; name[i] && i<sizeof(m->name)-1; i++)
	m->name[i] = name[i];
      m->name[i] = '\0';
    }
#endif
  memset((void*)addr, 0, size);
  mmap_insert_list(m, prev);
#if DEBUG_LEVEL>1
  _dl_mmap_list_regions(0);
#endif
  return (void*)addr;
}

/** Exit ldso. */
void
_dl_exit(int code)
{
  printf("ldso: Exiting with %d\n", code);
#if DEBUG_LEVEL>0
  enter_kdebug("stop");
#endif

  if (! l4ts_connected())
    {
      printf("SIMPLE_TS not found -- cannot send exit event");
      l4_sleep_forever();
    }

  l4ts_exit();
}

/** Emulation of sys_mmap(). */
void*
_dl_mmap(void *start, unsigned size, int prot, int flags, int fd,
	 unsigned offset)
{
  mmap_region_t *m;

  LOGd(INFO, "\033[%dm("l4_addr_fmt"-"l4_addr_fmt", fd=%d) called from "
            l4_addr_fmt"\033[m",
	    (flags & MAP_ANONYMOUS) ? 33 : 34,
	    (l4_addr_t)start, (l4_addr_t)start+size, fd,
	    (l4_addr_t)__builtin_return_address(0));

  if (flags & MAP_ANONYMOUS)
    {
      /* anonymous mmap, map memory */
      if (!(flags & MAP_FIXED))
	{
	  void *rc;
	  LOGd(INFO, "=> anonymous");
	  rc = mmap_anonymous(~0U, size, prot, 0, "[anonymous]");
	  return rc ? rc : (void*)-1;
	}
      else
	{
	  LOGd(INFO, "=> anonymous fixed "l4_addr_fmt, (l4_addr_t)start);
	}
    }

  LOGd(INFO, "=> file offset "l4_addr_fmt" size "l4_addr_fmt, 
       (l4_addr_t)offset, (l4_addr_t)size);
  if (!(flags & MAP_ANONYMOUS) && (open_fd != fd))
    {
      LOGd(DBG, "MAP_FD but no file open or other file");
      return (void*)-1;
    }
  if (offset & (~L4_PAGEMASK))
    {
      LOGd(DBG, "offset ("l4_addr_fmt") is not page-aligned", 
	   (l4_addr_t)offset);
      return (void*)-1;
    }
  if (!(flags & MAP_ANONYMOUS) && offset+size > open_size)
    {
      LOGd(DBG, "Trying to mmap beyond end of file");
      return (void*)-1;
    }
  if (!(m = mmap_find_region((l4_addr_t)start, size)))
    {
      LOGd(DBG, "Still no memory backed!");
      return (void*)-1;
    }

  if (flags & MAP_ANONYMOUS)
    memset(start, 0, size); // XXX we should check if we overwrite the file
  else
    {
      memcpy(start, (void*)(open_addr+offset), size);
#if DEBUG_LEVEL>0
      /* name region */
      memcpy(m->name, open_fname, 32);
#endif
    }

  return start;
}

/** Emulation of sys_munmap(). */
int
_dl_munmap(void *start, unsigned size)
{
  mmap_region_t *m;
  l4_addr_t addr;

  addr = l4_trunc_page((l4_addr_t)start);
  size = l4_round_page((l4_addr_t)start+size) - addr;

  LOGd(INFO, "\033[32m("l4_addr_fmt"-"l4_addr_fmt") called from "
            l4_addr_fmt"\033[m",
	    (l4_addr_t)start, (l4_addr_t)start+size,
	    (l4_addr_t)__builtin_return_address(0));

  if ((m = mmap_find_region(addr, size)))
    {
      if (size == m->size)
	{
	  LOGd(INFO, "closing ds %d", m->ds.id);
	  l4dm_close(&m->ds);
	  m->size = 0;
	  mmap_delete_list(m);
#if DEBUG_LEVEL>1
	  _dl_mmap_list_regions(0);
#endif
	  return 0;
	}
      printf("size != m->size\n");
      _dl_exit(1);
    }

#if DEBUG_LEVEL>1
  _dl_mmap_list_regions(0);
#endif
  LOGd(INFO, "=> failed");
  return -1;
}

void*
_dl_alloc_pages(l4_size_t size, l4_addr_t *phys, const char *name)
{
  return (void*)mmap_anonymous(~0U, size, 0, phys, name);
}

void
_dl_free_pages(void *addr, l4_size_t size)
{
  if (addr)
    _dl_munmap(addr, size);
}

/** Emulation of sys_open(). Get the whole file image from the file provider.
 *  The image is unmapped on sys_close(). */
int
_dl_open(const char *name, int flags, unsigned mode)
{
  DICE_DECLARE_ENV(env);
  mmap_region_t *m, *prev;
  const char *p, *r;
  char *q;
  static char fname[1024];
  int error;
  Elf32_Ehdr *e;

  LOGd(INFO, "\033[31m(%s,%x,%x) called from "l4_addr_fmt"\033[m",
	     name, flags, mode, (l4_addr_t)__builtin_return_address(0));

  if ((p = strstr(name, "/fprov/")))
    name = p+7;
  if (open_fd)
    {
      printf("open_fd != 0\n");
      _dl_exit(1);
    }

  /* first try to open without path */
  p = NULL;
  for (;;)
    {
      q = fname;
      r = name;
      if (p)
	{
	  if (!*p)
	    /* end-of-path reached */
	    break;
	  while (q < fname + sizeof(fname)-1 && *p != ':' && *p != '\0')
	    *(q++) = *(p++);
	  p++; /* skip ':' */
	  if (q < fname + sizeof(fname)-1)
	    *(q++) = '/';
	}
      while (q < fname + sizeof(fname)-1 && *r != '\0')
	*(q++) = *(r++);
      *q = '\0';
      /* get the file image from the file provider. */
      error = l4fprov_file_open_call(&global_env->fprov_id, fname,
				     &global_env->memserv_id,
				     L4DM_PINNED, &open_ds, &open_size, &env);
      if (!error && DICE_EXCEPTION_MAJOR(&env) == CORBA_NO_EXCEPTION)
	{
	  if (!(open_addr = mmap_find_free_addr(open_size, &prev)))
	    {
	      LOGd(DBG, "No free open address size "l4_addr_fmt,
		        (l4_addr_t)open_size);
	      l4dm_close(&open_ds);
	      break;
	    }
	  if (!(m = mmap_find_free_slot(prev)))
	    {
	      LOGd(DBG, "No free open slot (incease MMAP_ENTRIES!)");
	      l4dm_close(&open_ds);
	      break;
	    }
	  if ((error = l4dm_map_ds(&open_ds, 0,
				   open_addr, open_size, L4DM_RO)))
	    {
	      LOGd(DBG, "Error %d in open_map", error);
	      l4dm_close(&open_ds);
	      break;
	    }

	  e = (Elf32_Ehdr *)open_addr;
	  /* Check if the read file is actually an ELF file */
	  if (   e->e_ident[0] != ELFMAG0 || e->e_ident[1] != ELFMAG1
	      || e->e_ident[2] != ELFMAG2 || e->e_ident[3] != ELFMAG3)
	    {
	      printf("Invalid ELF image (char 1-4: %c%c%c%c)\n",
		     e->e_ident[0], e->e_ident[1], e->e_ident[2], e->e_ident[3]);
	      break;
	    }

	  m->addr = open_addr;
	  m->size = l4_round_page(open_size);
	  m->ds   = open_ds;
	  mmap_insert_list(m, prev);

	  open_offs = 0;
	  open_fd = ++open_fd_magic;
	  LOGd(INFO, "File at "l4_addr_fmt" size "l4_addr_fmt, 
	      open_addr, (l4_addr_t)open_size);

	  /* save file name */
	  for (q=fname; *q!='\0'; q++)
	    ;
	  q--;
	  while (*q!='/' && q>fname)
	    q--;
	  if (*q == '/')
	    q++;
#if DEBUG_LEVEL>0
	  strncpy(open_fname, q, sizeof(open_fname)-1);
	  open_fname[sizeof(open_fname)-1] = '\0';
	  memcpy(m->name, open_fname, sizeof(open_fname));
#endif
#if DEBUG_LEVEL>1
	  _dl_mmap_list_regions(0);
#endif
	  return open_fd;
	}
      if (DICE_HAS_EXCEPTION(&env))
	{
	  LOGd(DBG, "IPC error ");
	  break;
	}
      if (!p)
	/* not found directly, start searchin in path */
	p = global_env->libpath;
    }

  /* failure */
  return -1;
}

/** Emulation of sys_close(). The file image is unmapped and the dataspace
 *  is closed. */
void
_dl_close(int fd)
{
  mmap_region_t *m;

  LOGd(INFO, "\033[31m(%d) called\033[m", fd);
  if (fd != open_fd)
    {
      LOGd(DBG, "closing wrong file");
      return;
    }

  if (!(m = mmap_find_region(open_addr, open_size)))
    {
      LOGd(DBG, "open image not found");
      return;
    }

  mmap_delete_list(m);
#if DEBUG_LEVEL>1
  _dl_mmap_list_regions(0);
#endif
  l4dm_close(&open_ds);
  open_fd = 0;
}

/* Called only once by dl-elf to read the ELF header of a shared library.
 * The buffer was allocated using ANONYMOUSE mmap */
unsigned
_dl_read(int fd, void *buf, unsigned count)
{
  LOGd(INFO, "(size=%d, addr="l4_addr_fmt") called from "l4_addr_fmt,
      count, (l4_addr_t)buf, (l4_addr_t)__builtin_return_address(0));
  if (fd != open_fd)
    {
      LOGd(DBG, "file not open");
      _dl_exit(1);
    }
  if (open_size == 0)
    {
      LOGd(DBG, "no file open");
      return -1;
    }
  if (open_offs + count > open_size)
    count = open_size - open_offs;
  memcpy(buf, (void*)(open_addr+open_offs), count);
  open_offs += count;
  return count;
}

void
_dl_seek(int fd, unsigned pos)
{
  open_offs = pos;
}

int
_dl_mprotect(const void *addr, unsigned len, int prot)
{
  LOGd(INFO, "("l4_addr_fmt"-"l4_addr_fmt") called",
	    (l4_addr_t)addr, (l4_addr_t)addr+len);
  return -1;
}

unsigned short
_dl_getuid(void)
{
  LOGd(DBG, "called");
  return 0;
}

unsigned short
_dl_geteuid(void)
{
  LOGd(DBG, "called");
  return 0;
}

unsigned short
_dl_getgid(void)
{
  LOGd(DBG, "called");
  return 0;
}

unsigned short
_dl_getegid(void)
{
  LOGd(DBG, "called");
  return 0;
}

/* Called only once by ldso.c to dereference the symbolic link of the
 * interpreter name. */
int
_dl_readlink(const char *path, char *buf, unsigned size)
{
  LOGd(DBG, "(%s) called", path);
  return 0;
}

/* Called only by _dl_dprintf which only writes to STDERR */
int
_dl_write(int fd, const char *str, unsigned size)
{
  outnstring(str, size);
  return size;
}
