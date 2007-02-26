/*****************************************************************************/
/**
 * \file   l4ide/src/include/driver/notification.h
 * \brief  Processed notification stuff.
 *
 * \date   01/27/2004
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef _L4IDE_NOTIFICATION_H
#define _L4IDE_NOTIFICATION_H

/* Driver includes */
#include <driver/types.h>

/* prototypes */
int
l4ide_start_notification_thread(l4ide_driver_t * driver);

void
l4ide_shutdown_notification_thread(l4ide_driver_t * driver);

void
l4ide_init_notifications(void);

int
l4ide_do_notification(l4ide_driver_t * driver, l4_uint32_t handle,
                      l4_uint32_t status, l4_uint32_t error);


#endif /* !_L4IDE_NOTIFICATION_H */
