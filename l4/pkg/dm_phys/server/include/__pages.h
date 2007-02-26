/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/include/__pages.h
 * \brief  Page list management
 *
 * \date   08/04/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
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
#ifndef _DM_PHYS___PAGES_H
#define _DM_PHYS___PAGES_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>

/* private includes */
#include "__config.h"

/*****************************************************************************
 *** Types
 *****************************************************************************/

/**
 * Memory pool descriptor 
 */
typedef struct page_pool page_pool_t;

/**
 * Page area descriptor
 */
typedef struct page_area
{
  l4_addr_t          addr;        ///< area start address
  l4_size_t          size;        ///< area size

  l4_uint32_t        flags;       ///< area flags

  /* dataspace page area list */
  struct page_area * ds_next;

  /* sequential area list */
  struct page_area * area_prev;
  struct page_area * area_next;

  /* free list */
  struct page_area * free_prev;
  struct page_area * free_next;
} page_area_t;

#define AREA_USED          0x00000001

#define IS_USED_AREA(a)    ((a)->flags & AREA_USED)    ///< test if used area
#define IS_UNUSED_AREA(a)  (!IS_USED_AREA(a))          ///< test if unused area
#define SET_AREA_USED(a)   ((a)->flags |= AREA_USED)   ///< mark area used
#define SET_AREA_UNUSED(a) ((a)->flags &= ~AREA_USED)  ///< mark area unused

/**
 * Return map address of area 
 */
#define AREA_MAP_ADDR(a)     (DMPHYS_MEMMAP_START + (a)->addr)

/**
 * Return map address of phys. address
 */
#define MAP_ADDR(a)          (DMPHYS_MEMMAP_START + (a))

/**
 * Return phys. address of map address 
 */
#define PHYS_ADDR(addr)      ((l4_addr_t)(addr) - DMPHYS_MEMMAP_START)

/**
 * Memory pool descriptor
 */
struct page_pool
{
  int           pool;                              ///< pool number
  char *        name;                              ///< pool name

  /* bookkeeping */
  l4_size_t     size;                              ///< total size
  l4_size_t     free;                              ///< free memory
  l4_size_t     reserved;                          ///< reserved

  page_area_t * area_list;                         ///< page area list
  page_area_t * free_list[DMPHYS_NUM_FREE_LISTS];  ///< free lists
};

/* allocation priorities */
#define PAGES_USER       0    ///< normal user allocation
#define PAGES_INTERNAL   1    ///< internal allocation, ignore reservations

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

/* page area handling init */
int
dmphys_pages_init(void);

/* setup page pool descriptor */
void
dmphys_pages_init_pool(int pool, 
		       l4_size_t reserved, 
		       char * name);

/* page pools */
page_pool_t *
dmphys_get_page_pool(int pool);

page_pool_t *
dmphys_get_default_pool(void);

/* add free area */
int
dmphys_pages_add_free_area(int pool, 
			   l4_addr_t addr, 
			   l4_size_t size);

/* allocate pages */
int
dmphys_pages_allocate(page_pool_t * pool, 
		      l4_size_t size, 
		      l4_addr_t alignment, 
		      l4_uint32_t flags, 
		      int prio, 
		      page_area_t ** areas);

/* allocate page area */
int
dmphys_pages_allocate_area(page_pool_t * pool, 
			   l4_addr_t addr, 
			   l4_size_t size,
			   int prio, 
			   page_area_t ** area);

/* release pages */
void
dmphys_pages_release(page_pool_t * pool, 
		     page_area_t * areas);

/* try to enlarge page area */
int
dmphys_pages_enlarge(page_pool_t * pool, 
		     page_area_t * area, 
		     l4_size_t size, 
		     int prio);

/* add more pages to page list */
int
dmphys_pages_add(page_pool_t * pool, 
		 page_area_t * pages, 
		 l4_size_t size, 
		 int prio);

/* shrink page list */
int
dmphys_pages_shrink(page_pool_t * pool, 
		    page_area_t * pages, 
		    l4_size_t size);

/* find offset in page list */
L4_INLINE page_area_t *
dmphys_pages_find_offset(page_area_t * list, 
			 l4_offs_t offset, 
			 l4_offs_t * area_offset);

/* get number of page areas in dataspace list */
L4_INLINE int
dmphys_pages_get_num(page_area_t * list);

/* get overall size of page area list */
L4_INLINE l4_size_t
dmphys_pages_get_size(page_area_t * list);

/* DEBUG */
void
dmphys_pages_dump_used_pools(void);

void
dmphys_pages_dump_areas(page_pool_t * pool);

void 
dmphys_pages_dump_free(page_pool_t * pool);

void
dmphys_pages_list(page_area_t * list);

/*****************************************************************************
 *** implementation 
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Find page area which contains offset
 * 
 * \param  list          Page area list head
 * \param  offset        Offset
 * \retval area_offset   Offset in page area
 *	
 * \return Pointer to page area, NULL if \a offset points beyond the end of 
 *         the dataspace
 */
/*****************************************************************************/ 
L4_INLINE page_area_t *
dmphys_pages_find_offset(page_area_t * list, 
			 l4_offs_t offset, 
			 l4_offs_t * area_offset)
{
  page_area_t * pa = list;

  /* find page area which contains offset */
  while ((pa != NULL) && (offset >= pa->size))
    {
      offset -= pa->size;
      pa = pa->ds_next;
    }

  *area_offset = offset;
  return pa;
}

/*****************************************************************************/
/**
 * \brief  Return number of page areas in dataspace area list
 * 
 * \param  list          Area list head
 *	
 * \return Number of page areas in list.
 */
/*****************************************************************************/ 
L4_INLINE int
dmphys_pages_get_num(page_area_t * list)
{
  page_area_t * pa = list;
  int num = 0;
  
  /* count page areas */
  while (pa != NULL)
    {
      num++;
      pa = pa->ds_next;
    }

  return num;
}

/*****************************************************************************/
/**
 * \brief  Return memory size used in page area list
 * 
 * \param  list          Area list head
 *	
 * \return Overall size.
 */
/*****************************************************************************/ 
L4_INLINE l4_size_t
dmphys_pages_get_size(page_area_t * list)
{
  page_area_t * pa = list;
  l4_size_t size = 0;
  
  /* calculate size of page areas */
  while (pa != NULL)
    {
      size += pa->size;
      pa = pa->ds_next;
    }

  return size;
}

#endif /* _DM_PHYS___PAGES_H */
