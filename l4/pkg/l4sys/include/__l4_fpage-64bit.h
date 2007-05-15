#ifndef L4_FPAGE_H__
#define L4_FPAGE_H__


/*****************************************************************************
 *** L4 flexpages
 *****************************************************************************/

/**
 * L4 flexpage structure
 * \ingroup api_types_fpage
 */
typedef struct {
  unsigned grant:1;          ///< Grant page (send flexpage)
  unsigned write:1;          ///< Map writable (send flexpage)
  unsigned size:7;           ///< Flexpage size (log2)
  unsigned cache:3;          ///< Cachebility option
  unsigned long page:52;     ///< Page address
} l4_fpage_struct_t;

/**
 * L4 I/O flexpage structure
 * \ingroup api_types_fpage
 */
typedef struct {
  unsigned long grant:1;          ///< Grant I/O page (send I/O flexpage)
  unsigned long zero1:1;          ///< Unused (no write permissions, must be 0)
  unsigned long iosize:6;         ///< I/O flexpage size
  unsigned long zero2:4;          ///< Unused (must be 0)
  unsigned long iopage:16;        ///< I/O flexpage base address
  unsigned long f: 36;            ///< Unused, must be 0xF
} l4_iofpage_struct_t;

/**
 * L4 flexpage type
 * \ingroup api_types_fpage
 */
typedef union {
  l4_umword_t fpage;         ///< Plain 64 bit value
  l4_umword_t raw;
  l4_fpage_struct_t fp;      ///< Flexpage structure
  l4_iofpage_struct_t iofp;  ///< I/O Flexpage structure
} l4_fpage_t;

/** Constants for flexpages 
 * \ingroup api_types_fpage
 */
enum
{
  L4_WHOLE_ADDRESS_SPACE =64 /**< Whole address space size */
};

#include <l4/sys/__l4_fpage-common.h>

L4_INLINE l4_fpage_t
l4_fpage(unsigned long address, unsigned int size,
         unsigned char write, unsigned char grant)
{
  return ((l4_fpage_t){fp:{grant, write, size, 0,
                           (address & L4_PAGEMASK) >> 12 }});
}

L4_INLINE int
l4_is_io_page_fault(unsigned long address)
{
  return (address & 0xfffffffff0000f00UL) == 0xfffffffff0000000UL;
}

#endif
