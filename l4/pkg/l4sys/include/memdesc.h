#ifndef __L4SYS__MEMDESC_H__
#define __L4SYS__MEMDESC_H__

#include <l4/sys/kernel.h>

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


typedef struct
{
  l4_umword_t l, h;
} l4_kernel_info_mem_desc_t;


L4_INLINE
l4_kernel_info_mem_desc_t *
l4_kernel_info_get_mem_descs(l4_kernel_info_t *kip);

L4_INLINE
unsigned
l4_kernel_info_get_num_mem_descs(l4_kernel_info_t *kip);

L4_INLINE
void
l4_kernel_info_set_mem_desc(l4_kernel_info_mem_desc_t *md,
                            l4_addr_t start,
			    l4_addr_t end,
			    unsigned type,
			    unsigned virt,
			    unsigned sub_type);

L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_start(l4_kernel_info_mem_desc_t *md);

L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_end(l4_kernel_info_mem_desc_t *md);

L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_type(l4_kernel_info_mem_desc_t *md);

L4_INLINE
l4_umword_t
l4_kernel_info_get_mem_desc_subtype(l4_kernel_info_mem_desc_t *md);

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
  return kip->mem_info & ((1 << (sizeof(l4_umword_t)*4)) -1);
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
