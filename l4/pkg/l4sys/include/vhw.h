/*****************************************************************************/
/*!
 * \file    l4sys/include/vhw.h
 * \brief   Descriptors for virtual hardware (under UX).
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef _L4_SYS_VHW_H
#define _L4_SYS_VHW_H

#include <l4/sys/types.h>
#include <l4/sys/kernel.h>

/**
 * \defgroup api_vhw Virtual hardware devices
 * \ingroup api_types
 * \brief Virtual hardware devices, provided by Fiasco-UX
 */

/**
 * \brief Type of device
 * \ingroup api_vhw
 */
enum l4_vhw_entry_type {
  L4_TYPE_VHW_NONE,                        /**< None entry */
  L4_TYPE_VHW_FRAMEBUFFER,                 /**< Framebuffer device */
  L4_TYPE_VHW_INPUT,                       /**< Input device */
  L4_TYPE_VHW_NET,                         /**< Network device */
};

/**
 * \brief Description of a device
 * \ingroup api_vhw
 */
struct l4_vhw_entry {
  enum l4_vhw_entry_type type;             /**< Type of virtual hardware */
  l4_uint32_t            provider_pid;     /**< Host PID of the VHW provider */

  l4_addr_t              mem_start;        /**< Start of memory region */
  l4_addr_t              mem_size;         /**< Size of memory region */

  l4_uint32_t            irq_no;           /**< IRQ number */
  l4_uint32_t            fd;               /**< File descriptor */
};

/**
 * \brief Virtual hardware devices description
 * \ingroup api_vhw
 */
struct l4_vhw_descriptor {
  l4_uint32_t magic;                       /**< Magic */
  l4_uint8_t  version;                     /**< Version of the descriptor */
  l4_uint8_t  count;                       /**< Number of entries */
  l4_uint8_t  pad1;                        /**< padding \internal */
  l4_uint8_t  pad2;                        /**< padding \internal */

  struct l4_vhw_entry descs[];             /**< Array of device descriptions */
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
