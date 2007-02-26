/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_blk/lib/client/include/__libblk.h
 * \brief  Various lib stuff.
 *
 * \date   02/11/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _GENERIC_BLK___LIBBLK_H
#define _GENERIC_BLK___LIBBLK_H

/* library includes */
#include "__driver.h"

/* prototypes */
int
blkclient_start_notification_thread(blkclient_driver_t * driver);

void
blkclient_shutdown_notification_thread(blkclient_driver_t * driver);

#endif /* !_GENERIC_BLK___LIBBLK_H */
