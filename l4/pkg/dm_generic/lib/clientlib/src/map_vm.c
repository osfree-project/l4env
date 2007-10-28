/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/client-lib/src/map_vm.c
 * \brief  Generic dataspace manager client library, map implementation
 *
 * \date   01/08/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
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
#include <l4/l4rm/l4rm.h>

/* DMgeneric includes */
#include <l4/dm_generic/dm_generic.h>
#include "__debug.h"
#include "__map.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Calculate the max. alignment of an address in address range
 *
 * \param  start         Address range start address
 * \param  end           Address range end address
 *
 * \return Alignment (log2)
 *
 * Find alignment of the address with the max. alignment in the given address
 * range. It is calculated from:
 *
 *   (start != end) ? bsr((start - 1) xor end) : bsf(start)
 *
 * where:
 * - bsf(start) (bit scan forward) finds the least significant bit set
 *   => it's alignment of the start address
 * - bsr((start - 1) xor end) (bit scan reverse) finds the most significant
 *   bit set => find the most significant bit which is different in
 *   (start - 1) and end. The bit number is the max. alignment of an address
 *   in that address range.
 *
 * Preconditions:
 * - start/end must be page aligned
 */
/*****************************************************************************/
static inline int
__max_addr_align(l4_addr_t start, l4_addr_t end)
{
  if (start != end)
    return l4util_bsr((start - 1) ^ end);

  return (start == 0)
    ? L4_WHOLE_ADDRESS_SPACE
    : l4util_bsf(start);
}


/*****************************************************************************/
/**
 * \brief Map VM area
 *
 * \param  ptr           VM address
 * \param  size          Area size
 * \param  flags         Flags:
 *                       - #L4DM_RO          map read-only
 *                       - #L4DM_RW          map read/write
 *                       - #L4DM_MAP_PARTIAL allow partial mappings
 *                       - #L4DM_MAP_MORE    if possible, map more than the
 *                                           specified VM region
 *
 * \return 0 on success (mapped VM area), error code otherwise:
 *         - -#L4_EINVAL       Invalid vm region (i.e. no dataspace attached to
 *                             that region, but external pager etc.)
 *         - -#L4_EIPC         IPC error calling regione mapper /
 *                             dataspace manager
 *         - -#L4_ENOTFOUND    No dataspace attached to parts of the VM area
 *         - -#L4_EPERM        Permission denied
 *
 * Map the specified VM area. This will lookup the dataspaces which are
 * attached to the VM area and will call the dataspace managers to map the
 * dataspace pages.
 *
 * Flags:
 * - #L4DM_MAP_PARTIAL allow partial mappings of the VM area. If no
 *                     dataspace is attached to a part of the VM area,
 *                     just stop mapping and return without an error.
 * - #L4DM_MAP_MORE    if possible, map more than the specified VM region.
 *                     This allows l4dm_map to map more pages than specified
 *                     by \a ptr and \a size if a dataspace is attached to a
 *                     larger VM region.
 */
/*****************************************************************************/
int
l4dm_map(const void * ptr, l4_size_t size, l4_uint32_t flags)
{
  l4_addr_t addr;
  l4dm_dataspace_t ds;
  l4_offs_t offs,rcv_offs;
  l4_addr_t ds_map_addr,ds_map_end,fpage_addr;
  l4_size_t ds_map_size,fpage_size,map_size,size_mapped;
  l4_addr_t rcv_addr,rcv_end;
  int done,ret,rcv_size2;
  int align_start,align_end,align_size,align_addr;
  l4_threadid_t dummy;

  /* round addr / size to pagesize */
  addr = l4_trunc_page((l4_addr_t)ptr);
  size = l4_round_page((l4_addr_t)ptr + size) - addr;

  LOGdL(DEBUG_MAP, "map VM area 0x"l4_addr_fmt" - 0x"l4_addr_fmt,
        addr, addr + size);

  /* map, repeat until the requested area is mapped completely */
  done = 0;
  while (!done)
    {
      /* lookup address */
      ret = l4rm_lookup((void *)addr, &ds_map_addr, &ds_map_size,
                        &ds, &offs, &dummy);
      if (ret < 0)
        {
          if (ret == -L4_ENOTFOUND)
            {
              /* no dataspace attached to the VM addr */
              if (flags & L4DM_MAP_PARTIAL)
                return 0;
            }

          LOGdL(DEBUG_ERRORS,
                "libdm_generic: lookup address 0x"l4_addr_fmt" failed: %d!",
                addr, ret);
          return ret;
        }

      if (ret != L4RM_REGION_DATASPACE)
        {
          LOGdL(DEBUG_ERRORS,
                "trying to map non-dataspace region at addr 0x"l4_addr_fmt
                " (type %d)", addr, ret);
          return -L4_EINVAL;
        }

      ds_map_end = ds_map_addr + ds_map_size;

      LOGdL(DEBUG_MAP, "addr 0x"l4_addr_fmt", ds %u at "l4util_idfmt", offs 0x"
            l4_addr_fmt", map area 0x"l4_addr_fmt"-0x"l4_addr_fmt,
            addr, ds.id, l4util_idstr(ds.manager), offs,
            ds_map_addr, ds_map_end);

      /* calculate receive window */
      if (flags & L4DM_MAP_MORE)
        {
          /* find the max. receive window in the map area which includes ptr.
           * align_start is the max. alignment of an address in the address
           * range (ds_map_addr,addr), which is the max. size of an receive
           * window with a start address in that range, align_end is the
           * max. alignment of an address in the address range
           * (addr + L4_PAGESIZE,ds_map_end), which is the max. size of
           * a receive window with an end address in that area. The max.
           * receive window size is the minimum of align_start and align_end
           */
          align_start = __max_addr_align(ds_map_addr, addr);
          align_end = __max_addr_align(addr + L4_PAGESIZE, ds_map_end);
          rcv_size2 = (align_start < align_end) ? align_start : align_end;
#if DEBUG_MAP
          LOG_printf(" align start %d, align end %d => "
                     "receive window size %d\n",
                     align_start, align_end, rcv_size2);
#endif
        }
      else
        {
          /* calculate the max. receive window size with the start address at
           * ptr in the dataspace map area. */
          align_size = l4util_bsr(ds_map_end - addr);
          align_addr = l4util_bsf(addr);
          rcv_size2 = (align_addr < align_size) ? align_addr : align_size;
#if DEBUG_MAP
          LOG_printf(" align addr %d, align size %d => "
                     "receive window size %d\n",
                     align_addr, align_size, rcv_size2);
#endif
    }
      rcv_addr = addr & ~((1UL << rcv_size2) - 1);
      rcv_end = rcv_addr + (1UL << rcv_size2);
      rcv_offs = addr - rcv_addr;

      map_size = rcv_end - addr;
      if (map_size > size)
        map_size = size;

#if DEBUG_MAP
      LOG_printf(" receive window at 0x"l4_addr_fmt"-0x"l4_addr_fmt
                 ", size %d\n", rcv_addr, rcv_end, rcv_size2);
      LOG_printf(" rcv offs 0x"l4_addr_fmr", map size 0x%08x\n",
                 rcv_offs, map_size);
#endif

      /* map, allow partial mapping */
      ret = __do_map(&ds, offs, size, rcv_addr, rcv_size2, rcv_offs,
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
          LOG_printf("l4dm_map: addr 0x"l4_addr_fmt", ds %d at "l4util_idfmt \
                     ", offs 0x"l4_addr_fmt"\n", addr, ds.id,
                     l4util_idstr(ds.manager), offs);
          LOG_printf("dataspace map area 0x"l4_addr_fmt"-0x"l4_addr_fmt"\n",
                     ds_map_addr, ds_map_end);
          LOG_printf("receive window at 0x"l4_addr_fmt", size2 %d, rcv offs 0x"
                     l4_addr_fmt"\n", rcv_addr, rcv_size2, rcv_offs);
          LOGL("libdm_generic: map failed: %d!", ret);
#endif
          return ret;
        }
      size_mapped = fpage_size - (addr - fpage_addr);

      LOGdL(DEBUG_MAP,
            "got fpage fpage at 0x"l4_addr_fmt", size 0x"l4_addr_fmt
            ", size mapped 0x"l4_addr_fmt, fpage_addr, (l4_addr_t)fpage_size,
            (l4_addr_t)size_mapped);

      if (size_mapped >= size)
        done = 1;
      else
        {
          /* requested area not yet fully mapped */
          addr += size_mapped;
          size -= size_mapped;
        }
    }

  /* done */
  return 0;
}
