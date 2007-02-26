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
#include <l4/exec/exec.h>
#include <l4/util/l4_macros.h>
#include "dl-syscall.h"
#include <l4/generic_fprov/generic_fprov-client.h>

#include "infopage.h"
#include "emul_linux.h"

#define MMAP_ENTRIES	64

#if DEBUG_LEVEL>1
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
#endif
} mmap_region_t;

static mmap_region_t	mmap_regions[MMAP_ENTRIES];
       mmap_region_t	*mmap_region_list;

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
	return m;
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
mmap_list_regions(void)
{
#if DEBUG_LEVEL>0
  mmap_region_t *m;

  if (!mmap_region_list)
    {
      printf("  <region list empty>\n");
      return;
    }

  for (m=mmap_region_list; m; m=m->next)
    printf("  %08x-%08x  %s\n", m->addr, m->addr+m->size, m->name);
#endif
}

/** Map anonymous memory at a free place. */
static l4_addr_t
mmap_anonymous(l4_addr_t want_addr, l4_size_t size, int prot)
{
  int error;
  l4_addr_t addr;
  mmap_region_t *prev, *m;

  size = l4_round_page(size);
  if (want_addr == ~1U)
    {
      if (!(addr = mmap_find_free_addr(size, &prev)))
	{
#if DEBUG_LEVEL>0
	  printf("No free mmap address size %08x\n", size);
	  enter_kdebug("stop");
#endif
	  return (l4_addr_t)-1;
	}
    }
  else
    {
      if (!(addr = mmap_test_free_addr(want_addr, size, &prev)))
	{
#if DEBUG_LEVEL>0
	  printf("Address at %08x size %08x not free\n", want_addr, size);
	  enter_kdebug("stop");
#endif
	  return (l4_addr_t)-1;
	}
    }
  if (!(m = mmap_find_free_slot(prev)))
    {
#if DEBUG_LEVEL>0
      printf("No free mmap slot (incease MMAP_ENTRIES)\n");
      enter_kdebug("stop");
#endif
      return (l4_addr_t)-1;
    }
  if ((error = l4dm_mem_open(global_env->memserv_id, size,
			     0, 0, "anon memory", &m->ds)))
    {
#if DEBUG_LEVEL>0
      printf("Error %d in mmap_open\n", error);
      enter_kdebug("stop");
#endif
      return (l4_addr_t)-1;
    }
  if ((error = l4dm_map_ds(&m->ds, 0, addr, size, L4DM_RW)))
    {
#if DEBUG_LEVEL>0
      printf("Error %d in mmap_map\n", error);
      enter_kdebug("stop");
#endif
      l4dm_close(&m->ds);
      return (l4_addr_t)-1;
    }
  m->addr    = addr;
  m->size    = size;
#if DEBUG_LEVEL>0
  memcpy(m->name, "[anonymous]", sizeof("[anonymous]"));
#endif
  memset((void*)addr, 0, size);
  mmap_insert_list(m, prev);
#if DEBUG_LEVEL>1
  mmap_list_regions();
#endif
  return addr;
}

/** Exit ldso. */
void
_dl_exit(int code)
{
  outstring("_dl_exit called\n");
  enter_kdebug("stop");
}

/** Emulation of sys_mmap(). */
void*
_dl_mmap(void *start, unsigned size, int prot, int flags, int fd,
	 unsigned offset)
{
  mmap_region_t *m;

  LOGd(DBG, "\033[%dm(%08x-%08x, fd=%d) called from %08x\033[m",
	    (flags & MAP_ANONYMOUS) ? 33 : 34,
	    (unsigned)start, (unsigned)start+size, fd,
	    (unsigned)__builtin_return_address(0));

  if (flags & MAP_ANONYMOUS)
    {
      /* anonymous mmap, map memory */
      if (!(flags & MAP_FIXED))
	{
	  LOGd(DBG, "=> anonymous");
	  return (void*)mmap_anonymous(~1U, size, prot);
	}
      else
	{
	  LOGd(DBG, "=> anonymous fixed %08x", (unsigned)start);
	}
    }

  LOGd(DBG, "=> file offset %08x size %08x", offset, size);
  if (!(flags & MAP_ANONYMOUS) && (open_fd != fd))
    {
      LOGd(DBG, "MAP_FD but no file open or other file");
      return (void*)-1;
    }
  if (offset & (~L4_PAGEMASK))
    {
      LOGd(DBG, "offset (%08x) is not page-aligned", offset);
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

  LOGd(DBG, "\033[32m(%08x-%08x) called from %08x\033[m",
	    (unsigned)start, (unsigned)start+size,
	    (unsigned)__builtin_return_address(0));

  if ((m = mmap_find_region(addr, size)))
    {
      if (size == m->size)
	{
	  LOGd(DBG, "closing ds");
	  l4dm_close(&m->ds);
	  m->size = 0;
	  mmap_delete_list(m);
#if DEBUG_LEVEL>1
	  mmap_list_regions();
#endif
	  return 0;
	}
      printf("size != m->size\n");
      enter_kdebug("stop");
    }

#if DEBUG_LEVEL>1
  mmap_list_regions();
#endif
  LOGd(DBG, "=> failed");
  return -1;
}

/** Emulation of sys_open(). Get the whole file image from the file provider.
 *  The image is unmapped on sys_close(). */
int
_dl_open(const char *name, int flags, unsigned mode)
{
  CORBA_Environment _env = dice_default_environment;
  mmap_region_t *m, *prev;
  const char *p, *r;
  char *q;
  static char fname[1024];
  int error;

  LOGd(DBG, "\033[31m(%s,%x,%x) called from %08x\033[m",
	     name, flags, mode, (unsigned)__builtin_return_address(0));

  if ((p = strstr(name, "/fprov/")))
    name = p+7;
  if (open_fd)
    {
      printf("open_fd != 0\n");
      enter_kdebug("stop");
      return -1;
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
	}
      while (q < fname + sizeof(fname)-1 && *r != '\0')
	*(q++) = *(r++);
      *q = '\0';
      /* get the file image from the file provider. */
      error = l4fprov_file_open_call(&global_env->fprov_id, fname,
				     &global_env->memserv_id,
				     0, &open_ds, &open_size, &_env);
      if (!error && _env.major == CORBA_NO_EXCEPTION)
	{
	  if (!(open_addr = mmap_find_free_addr(open_size, &prev)))
	    {
	      LOGd(DBG, "No free open address size %08x", open_size);
	      l4dm_close(&open_ds);
	      break;
	    }
	  if (!(m = mmap_find_free_slot(prev)))
	    {
	      LOGd(DBG, "No free open slot (incease MMAP_ENTRIES)");
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
	  m->addr = open_addr;
	  m->size = l4_round_page(open_size);
	  m->ds   = open_ds;
	  mmap_insert_list(m, prev);

	  open_offs = 0;
	  open_fd = ++open_fd_magic;
	  LOGd(DBG, "File at %08x size %08x", open_addr, open_size);

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
	  mmap_list_regions();
#endif
	  return open_fd;
	}
      if (_env.major != CORBA_NO_EXCEPTION)
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

  LOGd(DBG, "\033[31m(%08x) called\033[m", fd);
  if (fd != open_fd)
    {
#if DEBUG_LEVEL>0
      printf("closing wrong file\n");
      enter_kdebug("stop");
#endif
      return;
    }

  if (!(m = mmap_find_region(open_addr, open_size)))
    {
#if DEBUG_LEVEL>0
      printf("open image not found");
      enter_kdebug("stop");
#endif
      return;
    }

  mmap_delete_list(m);
#if DEBUG_LEVEL>1
  mmap_list_regions();
#endif
  l4dm_close(&open_ds);
  open_fd = 0;
}

/* Called only once by dl-elf to read the ELF header of a shared library.
 * The buffer was allocated using ANONYMOUSE mmap */
unsigned
_dl_read(int fd, void *buf, unsigned count)
{
  LOGd(DBG, "(%d) called from %08x",
      count, (unsigned)__builtin_return_address(0));
  if (open_size == 0)
    {
      LOGd(DBG, " no file open");
      return -1;
    }
  if (open_offs + count > open_size)
    count = open_size - open_offs;
  memcpy(buf, (void*)(open_addr+open_offs), count);
  open_offs += count;
  return count;
}

int
_dl_mprotect(const void *addr, unsigned len, int prot)
{
  LOGd(DBG, "(%08x-%08x) called",
	    (unsigned)addr, (unsigned)addr+len);
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
