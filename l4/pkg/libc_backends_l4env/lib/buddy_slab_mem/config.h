/*!
 * \file   libc_backends_l4env/lib/buddy_slab_mem/config.h
 * \brief  backend lib logging configuration
 *
 * \date   08/19/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __LIBC_BACKENDS_L4ENV_LIB_BUDDY_SLAB_MEM_CONFIG_H_
#define __LIBC_BACKENDS_L4ENV_LIB_BUDDY_SLAB_MEM_CONFIG_H_

/* Malloc debugging */
#define LOG_MALLOC_INIT		0
#define LOG_MALLOC_MALLOC	0
#define LOG_MALLOC_FREE		0
#define LOG_MALLOC_SLAB		0

/* Buddy debugging */
#define LOG_BUDDY_CALL_ALLOC	0
#define LOG_BUDDY_CALL_FREE	0
#define LOG_BUDDY_ALLOC		0
#define LOG_BUDDY_FREE		0

#endif
