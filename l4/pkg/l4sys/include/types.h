/*****************************************************************************/
/*!
 * \file    l4sys/include/types.h
 * \brief   Common L4 types
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef BASE_L4_TYPES_H__
#define BASE_L4_TYPES_H__

#include <l4/sys/l4int.h>
#include <l4/sys/compiler.h>



/**
 * Quota type structure
 * \ingroup api_types
 */
typedef union l4_quota_desc_t
{
  l4_umword_t raw;              /**< raw value */
  struct
  {
    unsigned long id: 12;      /**< id value */
    unsigned long amount: 16;  /**< amount value */
    unsigned long cmd: 4;      /**< cmd value */
  } q;                         /**< quota structure */
} l4_quota_desc_t;

enum {
  L4_KQUOTA_CMD_NOOP,
  L4_KQUOTA_CMD_SHARE,
  L4_KQUOTA_CMD_NEW
};

#define L4_INVALID_KQUOTA  ((l4_quota_desc_t){ .raw = 0 })  ///< Invalid kernel quota

/**
 * Message tag for IPC operations.
 * \ingroup api_types
 *
 * All predefined protocols used by the kernel.
 */
enum l4_msgtag_protocol
{
  L4_MSGTAG_IRQ           = -1L, ///< IRQ message
  L4_MSGTAG_PAGE_FAULT    = -2L, ///< Page fault message
  L4_MSGTAG_PREEMPTION    = -3L, ///< Preemption message
  L4_MSGTAG_SYS_EXCEPTION = -4L, ///< System exception
  L4_MSGTAG_EXCEPTION     = -5L, ///< Exception
  L4_MSGTAG_SIGMA0        = -6L, ///< Sigma0 protocol
  L4_MSGTAG_IO_PAGE_FAULT = -8L, ///< I/O page fault message
  L4_MSGTAG_CAP_FAULT     = -9L, ///< Capability fault message
};


/**
 * Message tag for IPC operations.
 * \ingroup api_types
 *
 * Describes the details of an IPC operation, in particular the
 * which parts of the UTCB have to be transmitted, and also flags
 * to anbale realtime and FPU extensions.
 *
 * The message tag also contains a user-defined label that could be used
 * to specify a protocol ID. Some negative values are reseved for kernel
 * protocols such as page faults and exceptions.
 *
 * The type must be treated completely opaque.
 */
typedef struct l4_msgtag_t
{
  l4_mword_t raw;   ///< raw value
#ifdef __cplusplus
  long label() const { return raw >> 16; }
  unsigned words() const { return raw & 0x3f; }
  unsigned items() const { return (raw >> 6) & 0x3f; }
  unsigned flags() const { return raw & 0xf000; }
  bool is_irq() const { return label() == L4_MSGTAG_IRQ; }
  bool is_page_fault() const { return label() == L4_MSGTAG_PAGE_FAULT; }
  bool is_preepmption() const { return label() == L4_MSGTAG_PREEMPTION; }
  bool is_sys_exception() const { return label() == L4_MSGTAG_SYS_EXCEPTION; }
  bool is_exception() const { return label() == L4_MSGTAG_EXCEPTION; }
  bool is_sigma0() const { return label() == L4_MSGTAG_SIGMA0; }
  bool is_io_page_fault() const { return label() == L4_MSGTAG_IO_PAGE_FAULT; }
  bool is_cap_fault() const { return label() == L4_MSGTAG_CAP_FAULT; }
#endif
} l4_msgtag_t;



/**
 * Create a message tag from the specified values.
 *
 * \param label the user-defined label
 * \param words the number of utyped words copied from the UTCB
 * \param items the number of typed items (e.g., flex pages) within the UTCB
 * \param flags the IPC flags for realtime and FPU extensions
 */
L4_INLINE l4_msgtag_t l4_msgtag(long label, unsigned words, unsigned items,
                                unsigned flags);

/**
 * Get the label of tag.
 * \param t the tag.
 */
L4_INLINE long l4_msgtag_label(l4_msgtag_t t);

/**
 * Get the number of words.
 * \param t the tag
 */
L4_INLINE unsigned l4_msgtag_words(l4_msgtag_t t);

/**
 * Get the number of items.
 * \param t the tag
 */
L4_INLINE unsigned l4_msgtag_items(l4_msgtag_t t);

/**
 * Get the flags.
 * \param t the tag
 */
L4_INLINE unsigned l4_msgtag_flags(l4_msgtag_t t);

/**
 * Was the message an IRQ.
 * \param t the tag
 */
L4_INLINE unsigned l4_msgtag_is_irq(l4_msgtag_t t);

/**
 * Was the message a page fault.
 * \param t the tag
 */
L4_INLINE unsigned l4_msgtag_is_page_fault(l4_msgtag_t t);

/**
 * Was the message a preemption IPC.
 * \param t the tag
 */
L4_INLINE unsigned l4_msgtag_is_preemption(l4_msgtag_t t);

/**
 * Was the message an system exception.
 * \param t the tag
 */
L4_INLINE unsigned l4_msgtag_is_sys_exception(l4_msgtag_t t);

/**
 * Was the message an exception.
 * \param t the tag
 */
L4_INLINE unsigned l4_msgtag_is_exception(l4_msgtag_t t);

/**
 * Was the message desired for sigma0.
 * \param t the tag
 */
L4_INLINE unsigned l4_msgtag_is_sigma0(l4_msgtag_t t);

/**
 * Was the message an I/O page fault.
 * \param t the tag
 */
L4_INLINE unsigned l4_msgtag_is_io_page_fault(l4_msgtag_t t);

/**
 * Was the message a cap fault.
 * \param t the tag
 */
L4_INLINE unsigned l4_msgtag_is_cap_fault(l4_msgtag_t t);




/* **************************************************************************
 * Implementations
 */

L4_INLINE
l4_msgtag_t l4_msgtag(long label, unsigned words, unsigned items,
                      unsigned flags)
{
  return (l4_msgtag_t){(label << 16) | (words & 0x3f) | ((items & 0x3f) << 6)
                       | (flags & 0xf000)};
}

L4_INLINE
long l4_msgtag_label(l4_msgtag_t t)
{ return t.raw >> 16; }

L4_INLINE
unsigned l4_msgtag_words(l4_msgtag_t t)
{ return t.raw & 0x3f; }

L4_INLINE
unsigned l4_msgtag_items(l4_msgtag_t t)
{ return (t.raw >> 6) & 0x3f; }

L4_INLINE
unsigned l4_msgtag_flags(l4_msgtag_t t)
{ return t.raw & 0xf000; }


L4_INLINE unsigned l4_msgtag_is_irq(l4_msgtag_t t)
{ return l4_msgtag_label(t) == L4_MSGTAG_IRQ; }

L4_INLINE unsigned l4_msgtag_is_page_fault(l4_msgtag_t t)
{ return l4_msgtag_label(t) == L4_MSGTAG_PAGE_FAULT; }

L4_INLINE unsigned l4_msgtag_is_preemption(l4_msgtag_t t)
{ return l4_msgtag_label(t) == L4_MSGTAG_PREEMPTION; }

L4_INLINE unsigned l4_msgtag_is_sys_exception(l4_msgtag_t t)
{ return l4_msgtag_label(t) == L4_MSGTAG_SYS_EXCEPTION; }

L4_INLINE unsigned l4_msgtag_is_exception(l4_msgtag_t t)
{ return l4_msgtag_label(t) == L4_MSGTAG_EXCEPTION; }

L4_INLINE unsigned l4_msgtag_is_sigma0(l4_msgtag_t t)
{ return l4_msgtag_label(t) == L4_MSGTAG_SIGMA0; }

L4_INLINE unsigned l4_msgtag_is_io_page_fault(l4_msgtag_t t)
{ return l4_msgtag_label(t) == L4_MSGTAG_IO_PAGE_FAULT; }

L4_INLINE unsigned l4_msgtag_is_cap_fault(l4_msgtag_t t)
{ return l4_msgtag_label(t) == L4_MSGTAG_CAP_FAULT; }

#endif /* ! BASE_L4_TYPES_H__ */
