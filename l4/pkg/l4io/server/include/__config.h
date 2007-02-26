/* $Id$ */
/*****************************************************************************/
/**
 * \file	l4io/server/include/__config.h
 *
 * \brief	L4Env l4io I/O Server Configuration
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#ifndef _IO___CONFIG_H
#define _IO___CONFIG_H

/**
 * \name Configuration
 *
 * 0 ... switch feature off
 * 1 ... on
 *
 * @{
 */
#define IORES_TOO_MUCH_POLICY	0	/**< skip client-check on release */
#define IOJIFFIES_L4SCSI	0	/**< use L4SCSI jiffies implementation */
#define IOJIFFIES_HZ		100	/**< jiffies update frequency */
#define IOJIFFIES_PERIOD	1000000/IOJIFFIES_HZ
					/**< jiffies period (method no.2) in us */

#define IO_MAX_THREADS		32	/**< maximum number of threads in l4io */
/** @} */
/**
 * \name DEBUG_ Macros
 *
 * 0 ... no output
 * 1 ... debugging output for this group
 *
 * @{
 */
#define DEBUG_PCI	1	/** debug pci module */
#define DEBUG_PCI_RW	1	/** debug pci rw config */
#define DEBUG_RES	1	/** debug res module */
#define DEBUG_REGDRV	1	/** debug driver registration */
#define DEBUG_MAP	1	/** debug mappings */

/** @} */


#endif /* !_IO___CONFIG_H */
