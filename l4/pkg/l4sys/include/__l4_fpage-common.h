/*!
 * \file    l4sys/include/__l4_fpage-common.h
 * \brief   Common fpage definitions
 * \ingroup api_calls
 */
#ifndef __L4_FPAGE_COMMON_H__
#define __L4_FPAGE_COMMON_H__

/**
 * \defgroup api_types_fpage Flexpages
 * \brief Fpages
 */

/**
 * Send flexpage types
 * \ingroup api_types_fpage
 */
typedef struct {
  l4_umword_t snd_base;      ///< Offset in receive window (send base)
  l4_fpage_t fpage;          ///< Source flexpage descriptor
} l4_snd_fpage_t;


/** Flexpage flags
 * \ingroup api_types_fpage
 */
enum 
{
  L4_FPAGE_RO    = 0, /**< Read-only flexpage  */
  L4_FPAGE_RW    = 1, /**< Read-write flexpage */
  L4_FPAGE_MAP   = 0, /**< Map flexpage        */
  L4_FPAGE_GRANT = 1  /**< Grant flexpage      */
};


/** Flexpage cacheability option
 * \ingroup api_types_fpage
 */
enum l4_fpage_cacheability_opt_t
{
  /** Enable the cacheablity option in a send fpage. */
  L4_FPAGE_CACHE_OPT   = 0x1,

  /** Cacheablity option to enable caches for the mapping. */
  L4_FPAGE_CACHEABLE   = 0x3,

  /** Cacheablity option to enable buffered writes for the mapping. */
  L4_FPAGE_BUFFERABLE  = 0x5,

  /** Cacheablity option to disable caching for the mapping. */
  L4_FPAGE_UNCACHEABLE = 0x1
};


/** Special constants for IO-flexpages 
 * \ingroup api_types_fpage
 */
enum
{
  /** Whole I/O address space size */
  L4_WHOLE_IOADDRESS_SPACE  = 16,

  /** Maximum I/O port address */
  L4_IOPORT_MAX             = (1L << L4_WHOLE_IOADDRESS_SPACE)
};


/** Special constants for cap-flexpages 
 * \ingroup api_types_fpage
 */
enum
{
  /** Whole I/O address space size */
  L4_WHOLE_CAPADDRESS_SPACE  = 11,

  /** Maximum I/O port address */
  L4_CAP_MAX                 = (1L << L4_WHOLE_CAPADDRESS_SPACE)
};


/**
 * \brief   Build flexpage descriptor
 * \ingroup api_types_fpage
 * 
 * \param   address      Flexpage source address
 * \param   size         Flexpage size (log2), #L4_WHOLE_ADDRESS_SPACE to
 *                       specify the whole address space (with \a address 0)
 * \param   write        Read-write flexpage (#L4_FPAGE_RW) or read-only
 *                       flexpage (#L4_FPAGE_RO)
 * \param   grant        Grant flexpage (#L4_FPAGE_GRANT) or map flexpage
 *                       (#L4_FPAGE_MAP)
 * \return  Flexpage descriptor.
 */
L4_INLINE l4_fpage_t
l4_fpage(unsigned long address, unsigned int size,
         unsigned char write, unsigned char grant);


/**
 * \brief   Build I/O flexpage descriptor
 * \ingroup api_types_fpage
 *
 * \param   port         I/O flexpage port base
 * \param   size         I/O flexpage size, #L4_WHOLE_IOADDRESS_SPACE to
 *                       specify the whole I/O address space (with \a port 0)
 * \param   grant        Grant flexpage (#L4_FPAGE_GRANT) or map flexpage
 *                       (#L4_FPAGE_MAP)
 * \return  I/O flexpage descriptor
 */
L4_INLINE l4_fpage_t
l4_iofpage(unsigned port, unsigned int size, unsigned char grant);


/**
 * \brief   Test if fault address describes I/O pagefault
 * \ingroup api_types_fpage
 *
 * \param   address      Fault address
 * \return  != 0 if \a address describes I/O pagefault, 0 if not
 */
L4_INLINE int
l4_is_io_page_fault(unsigned long address);


/**
 * \brief   Build Cap flexpage descriptor
 * \ingroup api_types_fpage
 *
 * \param   taskno       Capability task number
 * \param   order        Capability number of task, #L4_WHOLE_CAPADDRESS_SPACE
 *                       to specify the whole capability address space (with
 *                       \a taskno 0)
 * \param   grant        Grant flexpage (#L4_FPAGE_GRANT) or map flexpage
 *                       (#L4_FPAGE_MAP)
 * \return  Cap flexpage descriptor
 */
L4_INLINE l4_fpage_t
l4_capfpage(unsigned taskno, unsigned int order, unsigned char grant);

#endif /* ! __L4_FPAGE_COMMON_H__ */
