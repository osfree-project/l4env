/*
 * \brief   DDEUSB client library private header
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#ifndef L4_USB_DRIVER_LIBUSB_LOCAL_H
#define L4_USB_DRIVER_LIBUSB_LOCAL_H

#include <l4/dm_mem/dm_mem.h>
#include <l4/env/errno.h>
#include <l4/lock/lock.h>
#include <l4/log/l4log.h>
#include <l4/lock/lock.h>
#include <l4/names/libnames.h>
#include <l4/sys/types.h>
#include <l4/thread/thread.h>
#include <l4/sys/ipc.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>

#include <l4/usb/core-client.h>

#include <l4/usb/libddeusb.h>

#include "gadget-server.h" /* the notification server interface */
#include "usb_common.h" /* USB_NAMESERVER_NAME, */

#endif
