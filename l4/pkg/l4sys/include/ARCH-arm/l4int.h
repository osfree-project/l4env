/* $Id$ */
/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-arm/l4int.h
 * \brief   Fixed sized integer types, arm version
 * \ingroup api_types
 *
 * \date    11/12/2002
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef L4_SYS_L4INT_H
#define L4_SYS_L4INT_H

#define L4_MWORD_BITS		32

/* fixed sized data types */
typedef signed char             l4_int8_t;    /**< \ingroup api_types_common */
typedef unsigned char           l4_uint8_t;   /**< \ingroup api_types_common */
typedef signed short int        l4_int16_t;   /**< \ingroup api_types_common */
typedef unsigned short int      l4_uint16_t;  /**< \ingroup api_types_common */
typedef signed int              l4_int32_t;   /**< \ingroup api_types_common */
typedef unsigned int            l4_uint32_t;  /**< \ingroup api_types_common */
typedef signed long long        l4_int64_t;   /**< \ingroup api_types_common */
typedef unsigned long long      l4_uint64_t;  /**< \ingroup api_types_common */

/* some common data types */
typedef unsigned long           l4_addr_t;    /**< \ingroup api_types_common */
typedef unsigned long           l4_offs_t;    /**< \ingroup api_types_common */
typedef unsigned int            l4_size_t;    /**< \ingroup api_types_common */
typedef signed int              l4_ssize_t;   /**< \ingroup api_types_common */

typedef signed long             l4_mword_t;   /**< Signed machine word
					       **  \ingroup api_types_common
					       **/
typedef unsigned long           l4_umword_t;  /**< Unsigned machine word
					       **  \ingroup api_types_common
					       **/
/**
 * CPU clock type
 * \ingroup api_types_common
 */
typedef l4_uint64_t l4_cpu_time_t;

/**
 * Kernel clock type
 * \ingroup api_types_common
 */
typedef l4_uint64_t l4_kernel_clock_t;

#define L4_MWORD_BITS 32

#endif /* !L4_SYS_L4INT_H */
