/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-x86/vhw.h
 * \brief   Descriptors for virtual hardware (under UX).
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef _L4_SYS_VHW_H
#define _L4_SYS_VHW_H

#include <l4/sys/types.h>
#include <l4/sys/kernel.h>

enum l4_vhw_entry_type {
  L4_TYPE_VHW_NONE,
  L4_TYPE_VHW_FRAMEBUFFER,
  L4_TYPE_VHW_INPUT,
  L4_TYPE_VHW_NET,
};

struct l4_vhw_entry {
  enum l4_vhw_entry_type type;
  l4_uint32_t            provider_pid;

  l4_addr_t              mem_start;
  l4_addr_t              mem_size;

  l4_uint32_t            irq_no;
  l4_uint32_t            fd;
};

struct l4_vhw_descriptor {
  l4_uint32_t magic;
  l4_uint8_t  version;
  l4_uint8_t  count;
  l4_uint8_t  pad1;
  l4_uint8_t  pad2;

  struct l4_vhw_entry descs[];
};

enum {
  L4_VHW_MAGIC = 0x56687765,
};

static inline struct l4_vhw_descriptor *
l4_vhw_get(l4_kernel_info_t *kip)
{
  struct l4_vhw_descriptor *v
    = (struct l4_vhw_descriptor *)(((unsigned long)kip) + kip->vhw_offset);

  if (v->magic == L4_VHW_MAGIC)
    return v;

  return NULL;
}

static inline struct l4_vhw_entry *
l4_vhw_get_entry(struct l4_vhw_descriptor *v, int entry)
{
  return v->descs + entry;
}

static inline struct l4_vhw_entry *
l4_vhw_get_entry_type(struct l4_vhw_descriptor *v, enum l4_vhw_entry_type t)
{
  int i;
  struct l4_vhw_entry *e = v->descs;

  for (i = 0; i < v->count; i++, e++)
    if (e->type == t)
      return e;

  return NULL;
}

#endif /* ! _L4_SYS_VHW_H */
