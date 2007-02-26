/* $Id$ */
/*****************************************************************************/
/**
 * \file   semaphore/lib/include/__config.h
 * \brief  Library configuration.
 *
 * \date   12/30/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/
#ifndef _SEMAPHORE___CONFIG_H
#define _SEMAPHORE___CONFIG_H

/**
 * Semaphore thread priority
 */
#define L4SEMAPHORE_THREAD_PRIO       255

/**
 * Semaphore thread stack size 
 */
#define L4SEMAPHORE_THREAD_STACK_SIZE 4096

/**
 * Max. number of wait queue entries
 */
#define L4SEMAPHORE_MAX_WQ_ENTRIES    256

/**
 * Priority sort semaphore wait queues
 */ 
#define L4SEMAPHORE_SORT_WQ           1

#endif /* !_SEMAPHORE___CONFIG_H */
