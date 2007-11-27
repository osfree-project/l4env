/*!
 * \file    l4sys/include/__l4_fpage-32bit.h
 * \brief   Fpage definitions for 32bit systems
 * \ingroup api_calls
 */
#ifndef __L4_FPAGE_32BIT_H__
#define __L4_FPAGE_32BIT_H__

/*****************************************************************************
 *** L4 flexpages
 *****************************************************************************/

/**
 * \defgroup api_types_fpage_32 Fpage types for 32bit
 * \ingroup api_types_fpage
 */

/**
 * L4 flexpage structure
 * \ingroup api_types_fpage_32
 */
typedef struct {
  unsigned grant:1;          ///< Grant page (send flexpage)
  unsigned write:1;          ///< Map writable (send flexpage)
  unsigned size:6;           ///< Flexpage size (log2)
  unsigned zero:1;           ///< Unused (must be 0)
  unsigned cache:3;          ///< Cachebility options
  unsigned page:20;          ///< Page address
} l4_fpage_struct_t;

/**
 * L4 I/O flexpage structure
 * \ingroup api_types_fpage_32
 */
typedef struct {
  unsigned grant:1;          ///< Grant I/O page (send I/O flexpage)
  unsigned zero1:1;          ///< Unused (no write permissions, must be 0)
  unsigned iosize:6;         ///< I/O flexpage size (log2)
  unsigned zero2:4;          ///< Unused (must be 0)
  unsigned iopage:16;        ///< I/O flexpage base address
  unsigned f: 4;             ///< Unused, must be 0xF
} l4_iofpage_struct_t;

/**
 * L4 Capability flexpage structure
 * \ingroup api_types_fpage_32
 */
typedef struct {
  unsigned grant:1;          ///< Grant I/O page (send I/O flexpage)
  unsigned zero1:1;          ///< Unused (no write permissions, must be 0)
  unsigned order:6;          ///< Number of tasks (log2)
  unsigned one:4;            ///< Unused (must be =1!)
  unsigned taskno:16;        ///< Task number
  unsigned f: 4;             ///< Unused, must be 0xF
} l4_capfpage_struct_t;

/**
 * L4 flexpage type
 * \ingroup api_types_fpage_32
 */
typedef union {
  l4_umword_t fpage;          ///< Plain 32 bit value
  l4_umword_t raw;            ///< Raw value
  l4_fpage_struct_t fp;       ///< Flexpage structure
  l4_iofpage_struct_t iofp;   ///< I/O flexpage structure
  l4_capfpage_struct_t capfp; ///< Capability flexpage structure
} l4_fpage_t;

/** Constants for flexpages
 * \ingroup api_types_fpage_32
 */
enum
{
  L4_WHOLE_ADDRESS_SPACE = 32 /**< Whole address space size */
};

#include <l4/sys/__l4_fpage-common.h>

L4_INLINE l4_fpage_t
l4_fpage(unsigned long address, unsigned int size,
         unsigned char write, unsigned char grant)
{
  return ((l4_fpage_t){fp:{grant, write, size, 0, 0,
                           (address & L4_PAGEMASK) >> 12 }});
}


L4_INLINE int
l4_is_io_page_fault(unsigned long address)
{
  l4_fpage_t t;
  t.fpage = address;
  return t.iofp.f == 0xf && t.iofp.zero2 == 0;
}

L4_INLINE l4_fpage_t
l4_iofpage(unsigned port, unsigned int size,
           unsigned char grant)
{
  return ((l4_fpage_t){iofp:{grant, 0, size, 0, port, 0xf}});
}


L4_INLINE l4_fpage_t
l4_capfpage(unsigned taskno, unsigned int order,
            unsigned char grant)
{
  return ((l4_fpage_t){capfp:{grant, 0, order, 1, taskno, 0xf}});
}

#endif /* ! __L4_FPAGE_32BIT_H__ */
