/*!
 * \file l4sys/include/memdesc.h
 * \brief Memory description functions
 */
/* (c) 2007 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4SYS__MEMDESC_H__
#define __L4SYS__MEMDESC_H__

#include <l4/sys/kernel.h>

/**
 * \defgroup api_memdesc Memory descriptors.
 */

enum
{
  l4_mem_type_undefined    = 0x0,
  l4_mem_type_conventional = 0x1,
  l4_mem_type_reserved     = 0x2,
  l4_mem_type_dedicated    = 0x3,
  l4_mem_type_shared       = 0x4,

  l4_mem_type_bootloader   = 0xe,
  l4_mem_type_archspecific = 0xf,
};


/**
 * \brief   Memory descriptor data structure.
 * \ingroup api_memdesc
 */
typedef struct
{
  l4_umword_t l;             /**< low field */
  l4_umword_t h;             /**< high field */
} l4_kernel_info_mem_desc_t;


/**
 * \brief   Get pointer to memory descriptors from KIP.
 * \ingroup api_memdesc
 */
L4_INLINE
l4_kernel_info_mem_desc_t *
l4_kernel_info_get_mem_descs(l4_kernel_info_t *kip);

/**
 * \brief   Get number of memory descriptors.
 * \ingroup api_memdesc
 *
 * \return Number of memory descriptors.
 */
L4_INLINE
unsigned
l4_kernel_info_get_num_mem_descs(l4_kernel_info_t *kip);

/**
 * \brief   Populate a memory descriptor.
 * \ingroup api_memdesc
 *
 * \param md       Pointer to memory descriptor
 * \param start    Start of region
 * \param end      End of region
 * \param type     Type of region
 * \param virt     1 if virtual region, 0 if physical region
 * \param sub_type Sub type.
 */
L4_INLINE
void
l4_kernel_info_set_mem_desc(l4_kernel_info_mem_desc_t *md,
                            l4_addr_t start,
			    l4_addr_t end,
			    unsigned type,
			    unsigned virt,
			    unsigned sub_type);

/**
 * \brief   Get start value of memory descriptor
 * \ingroup api_memdesc
 *
 * \return Start value.
 */
L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_start(l4_kernel_info_mem_desc_t *md);

/**
 * \brief   Get end value of memory descriptor
 * \ingroup api_memdesc
 *
 * \return End value.
 */
L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_end(l4_kernel_info_mem_desc_t *md);

/**
 * \brief   Get type of memory descriptor
 * \ingroup api_memdesc
 *
 * \return Type value.
 */
L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_type(l4_kernel_info_mem_desc_t *md);

/**
 * \brief   Get sub-type of memory descriptor
 * \ingroup api_memdesc
 *
 * \return Sub-type value.
 */
L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_subtype(l4_kernel_info_mem_desc_t *md);

/**
 * \brief   Get virtual flag of memory descriptor.
 * \ingroup api_memdesc
 *
 * \return 1 if region is virtual, 0 if region is physical
 */
L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_is_virtual(l4_kernel_info_mem_desc_t *md);

/*************************************************************************
 * Implementations
 *************************************************************************/

L4_INLINE
l4_kernel_info_mem_desc_t *
l4_kernel_info_get_mem_descs(l4_kernel_info_t *kip)
{
  return (l4_kernel_info_mem_desc_t *)(((l4_addr_t)kip)
      + (kip->mem_info >> (sizeof(l4_umword_t) * 4)));
}

L4_INLINE
unsigned
l4_kernel_info_get_num_mem_descs(l4_kernel_info_t *kip)
{
  return kip->mem_info & ((1UL << (sizeof(l4_umword_t)*4)) -1);
}

L4_INLINE
void
l4_kernel_info_set_mem_desc(l4_kernel_info_mem_desc_t *md,
                            l4_addr_t start,
			    l4_addr_t end,
			    unsigned type,
			    unsigned virt,
			    unsigned sub_type)
{
  md->l = (start & ~0x3ffUL) | (type & 0x0f) | ((sub_type << 4) & 0x0f0)
    | (virt ? 0x200 : 0x0);
  md->h = end;
}


L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_start(l4_kernel_info_mem_desc_t *md)
{
  return md->l & ~0x3ffUL;
}

L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_end(l4_kernel_info_mem_desc_t *md)
{
  return md->h | 0x3ffUL;
}

L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_type(l4_kernel_info_mem_desc_t *md)
{
  return md->l & 0xf;
}

L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_subtype(l4_kernel_info_mem_desc_t *md)
{
  return (md->l & 0xf0) >> 4;
}

L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_is_virtual(l4_kernel_info_mem_desc_t *md)
{
  return md->l & 0x200;
}

#endif /* ! __L4SYS__MEMDESC_H__ */
