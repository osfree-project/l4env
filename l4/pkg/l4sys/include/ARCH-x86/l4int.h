/* $Id$ */
/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-x86/l4int.h
 * \brief   Fixed sized integer types, x86 version
 * \ingroup api_types
 *
 * \date    11/12/2002
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _L4_SYS_L4INT_H
#define _L4_SYS_L4INT_H

#define L4_MWORD_BITS		32            /**< Size of machine words in bits */

/* fixed sized data types */
typedef signed char             l4_int8_t;    /**< signed 8bit value */
typedef unsigned char           l4_uint8_t;   /**< unsigned 8bit value */
typedef signed short int        l4_int16_t;   /**< signed 16bit value */
typedef unsigned short int      l4_uint16_t;  /**< unsigned 16bit value */
typedef signed int              l4_int32_t;   /**< signed 32bit value */
typedef unsigned int            l4_uint32_t;  /**< unsigned 32bit value */
typedef signed long long        l4_int64_t;   /**< signed 64bit value */
typedef unsigned long long      l4_uint64_t;  /**< unsigned 64bit value */

/* some common data types */
typedef unsigned long           l4_addr_t;    /**< address type */
typedef unsigned long           l4_offs_t;    /**< address offset type  */
typedef unsigned int            l4_size_t;    /**< unsigned size type */
typedef signed int              l4_ssize_t;   /**< signed size type */

typedef signed long             l4_mword_t;   /**< Signed machine word **/
typedef unsigned long           l4_umword_t;  /**< Unsigned machine word **/

typedef l4_uint64_t l4_cpu_time_t;            /**< CPU clock type */

typedef l4_uint64_t l4_kernel_clock_t;        /**< Kernel clock type */

#endif /* !_L4_SYS_L4INT_H */
