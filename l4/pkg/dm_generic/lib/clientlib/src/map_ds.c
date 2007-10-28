/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/map_ds.c
 * \brief  Generic dataspace manager client library, map implementation
 *
 * \date   05/12/2005
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/util/bitops.h>

/* DMgeneric includes */
#include <l4/dm_generic/dm_generic.h>
#include "__debug.h"
#include "__map.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Map VM area
 *
 * \param  ds            Dataspace descriptor
 * \param  offs          Offset in dataspace
 * \param  addr          Map address
 * \param  size          Map size
 * \param  flags         Flags:
 *                       - #L4DM_RO          map read-only
 *                       - #L4DM_RW          map read/write
 *                       - #L4DM_MAP_PARTIAL allow partial mappings
 *
 * \return 0 on success (mapped VM area), error code otherwise:
 *         - -#L4_EINVAL       Invalid vm region (i.e. no dataspace attached to
 *                             that region, but external pager etc.)
 *         - -#L4_EIPC         IPC error calling regione mapper /
 *                             dataspace manager
 *         - -#L4_ENOTFOUND    No dataspace attached to parts of the VM area
 *         - -#L4_EPERM        Permission denied
 *
 * Map the specified area of the specifed dataspace. A better IDL wrapper.
 */
/*****************************************************************************/
int
l4dm_map_ds(const l4dm_dataspace_t *ds, l4_offs_t offs,
            l4_addr_t addr, l4_size_t size, l4_uint32_t flags)
{
  l4_offs_t rcv_offs;
  l4_addr_t map_addr,map_end,fpage_addr;
  l4_size_t map_size,map_sz,fpage_size,size_mapped;
  l4_addr_t rcv_addr,rcv_end;
  int ret,rcv_size2;
  int align_size,align_addr;

  /* round addr / size to pagesize */
  map_addr = l4_trunc_page(addr);
  map_size = l4_round_page(addr+size) - map_addr;
  map_end  = map_addr + map_size;

  LOGdL(DEBUG_MAP, "map ds area 0x"l4_addr_fmt" - 0x"l4_addr_fmt,
        map_addr, map_end);

  /* map, repeat until the requested area is mapped completely */
  for (;;)
    {
      LOGdL(DEBUG_MAP, "ds %u at "l4util_idfmt", offs 0x%08lx, " \
            "map area 0x%08lx-0x%08lx", ds->id, l4util_idstr(ds->manager),
            offs, map_addr, map_end);

      /* calculate the max. receive window size with the start address at
       * addr in the dataspace map area. */
      align_size = l4util_bsr(map_end - map_addr);
      align_addr = l4util_bsf(map_addr);
      rcv_size2  = (align_addr < align_size) ? align_addr : align_size;
#if DEBUG_MAP
      LOG_printf(" align addr %d, align size %d => receive window size %d\n",
                 align_addr, align_size, rcv_size2);
#endif
      rcv_addr = map_addr & ~((1UL << rcv_size2) - 1);
      rcv_end  = rcv_addr + (1UL << rcv_size2);
      rcv_offs = map_addr - rcv_addr;
      map_sz   = rcv_end  - map_addr;
      if (map_sz > size)
        map_sz = size;

#if DEBUG_MAP
      LOG_printf(" receive window at 0x"l4_addr_fmt"-0x"l4_addr_fmt
                 ", size %d\n", rcv_addr, rcv_end, rcv_size2);
      LOG_printf(" rcv offs 0x"l4_addr_fmt", map size 0x"l4_addr_fmt"\n",
                 rcv_offs, map_sz);
#endif

      /* map, allow partial mapping */
      ret = __do_map(ds, offs, map_sz, rcv_addr, rcv_size2, rcv_offs,
                     flags | L4DM_MAP_PARTIAL, &fpage_addr, &fpage_size);
      if (ret < 0)
        {
          /* map failed */
          if (ret == -L4_EINVAL_OFFS)
            {
              /* invalid offset in dataspace, this can happen if the
               * dataspace which is attached to the VM region is smaller
               * than the size of the VM region */
              if (flags & L4DM_MAP_PARTIAL)
                return 0;
            }

#if DEBUG_ERRORS
          LOG_printf("l4dm_map_ds: addr 0x"l4_addr_fmt", ds %d at "l4util_idfmt
                     ", offs 0x"l4_addr_fmt"\n", addr, ds->id,
                     l4util_idstr(ds->manager), offs);
          LOG_printf("dataspace map area 0x"l4_addr_fmt"-0x"l4_addr_fmt"\n",
                     map_addr, map_end);
          LOG_printf("receive window at 0x"l4_addr_fmt
                     ", size2 %d, rcv offs 0x"l4_addr_fmt"\n",
                     rcv_addr, rcv_size2, rcv_offs);
          LOGL("libdm_generic: map failed: %d!", ret);
#endif
          return ret;
        }
      size_mapped = fpage_size - (map_addr - fpage_addr);

      LOGdL(DEBUG_MAP,
            "got fpage fpage at 0x"l4_addr_fmt", size 0x"l4_addr_fmt
            ", size mapped 0x"l4_addr_fmt, fpage_addr, (l4_addr_t)fpage_size,
            (l4_addr_t)size_mapped);

      if (size_mapped >= map_size)
        return 0;

      /* requested area not yet fully mapped */
      map_addr += size_mapped;
      map_size -= size_mapped;
      offs     += size_mapped;
    }
}
