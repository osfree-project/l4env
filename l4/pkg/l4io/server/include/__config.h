/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/include/__config.h
 * \brief  L4Env l4io I/O Server Configuration
 *
 * \date   2007-03-23
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __L4IO_SERVER_INCLUDE___CONFIG_H_
#define __L4IO_SERVER_INCLUDE___CONFIG_H_

/**
 * \name Configuration
 *
 * 0 ... switch feature off
 * 1 ... on
 *
 * @{
 */
#define IORES_TOO_MUCH_POLICY   0   /**< skip client-check on release */
#define IOJIFFIES_L4SCSI        0   /**< use L4SCSI jiffies implementation */
#define IOJIFFIES_HZ            100 /**< jiffies update frequency */
#define IOJIFFIES_PERIOD        1000000/IOJIFFIES_HZ
                                    /**< jiffies period (method no.2) in us */

#define IO_MAX_THREADS          32  /**< maximum number of threads in l4io */
/** @} */
/**
 * \name DEBUG_ Macros
 *
 * 0 ... no output
 * 1 ... debugging output for this group
 *
 * @{
 */
#define DEBUG_ERRORS    1  /** verbose errror handling */

#define DEBUG_PCI       0  /** debug pci module */
#define DEBUG_PCI_RW    0  /** debug pci rw config */
#define DEBUG_RES       0  /** debug res module */
#define DEBUG_REGDRV    0  /** debug driver registration */
#define DEBUG_MAP       0  /** debug mappings */
/** @} */

#define IO_REQUEST_PAGE 1  /**< request 4K MMIO pages from pager */
#define IO_SEND_PAGE    1  /**< send 4K MMIO pages on requests */

#endif
