/*
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */



#ifndef L4_USB_SERVER_USB_LOCAL_H
#define L4_USB_SERVER_USB_LOCAL_H

#include <linux/usb.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/wait.h>

#include <l4/dde/linux26/dde26.h>
#include <l4/log/l4log.h>
#include <l4/sys/types.h>
#include <l4/l4rm/l4rm.h>
#include <l4/sys/ipc.h>
#include <l4/util/util.h>
#include <l4/env/errno.h>
#include <l4/lock/lock.h>
#include <l4/sys/types.h>
#include <l4/thread/thread.h>
#include <l4/util/l4_macros.h>
#include <l4/usb/usb_types.h>
#include "usb_common.h" /* USB_NAMESERVER_NAME, */
#include <l4/usb/core-client.h>
#include <l4/usb/core-server.h>
#include <l4/usb/gadget-client.h>

#define DDEUSB_CLIENT_MAX_URBS 256
#define DDEUSB_CLIENT_MAX_DEVS 30
#define DDEUSB_MAX_CLIENTS 10


#define DDEUSB_DEBUG 0
#define DDEUSB_MEASURE 0



#define CLIENT_UNUSED 0            /* the client structure is unused and  */
                                   /* may be used by a new client         */

#define CLIENT_ACTIVE 1            /* the client is active, he is allowed to */
                                   /* send urbs and is able to recieve       */
                                   /* completions                            */

#define CLIENT_CLEANUP 2           /* the client is not active anymore */
                                   /* he may not send urbs nor recieve */
                                   /* completions anymore              */



/* TYPES */

struct ddeusb_client;

struct ddeusb_urb_completion_handler_context
{
	int urb_id;
	unsigned int is_dma;
	struct ddeusb_client * client;
	struct ddeusb_dev *dev;

	int d_urb_id;

	unsigned int transfer_flags;
	struct list_head list;

	/* for chaching */
	int number_of_frames;
};


struct ddeusb_dev {
	struct list_head list;

	unsigned int dev_id; /* this is the device id on client side */

	struct usb_interface * intf; /* The interface the driver is bound to */

	struct usb_device_id id; /* The device id of this device
				  * (needed for matching with client)*/
	struct ddeusb_client * client; /* the client the device is bound to
					* (if bound) */

	struct  work_struct work;                  /* Work struct and data for doing synchronous calls */

	__u16 alternate;                           /* the data for the work */
	__u16 interface;

	int urb_id;
	int pipe;
};


struct ddeusb_client {
	atomic_t ref;              /* a refernce counter to this client */

	int status;                /* is this client still active */
	                           /* once unregister is called this will be false*/
	                           /* and all calls from this client will be rejected */

	l4_threadid_t th;          /* The thread id of the notification thread of the client */
	                           /* The client will also be identified by this this id (i.e by the client id) */



	const char *name;          /* The name of the client (not needed)*/

	struct ddeusb_dev *                  /* The devices which are served */
	dev[DDEUSB_CLIENT_MAX_DEVS];         /* by this client */
	struct list_head ids;      /* the device ids which the client
				    * is able to serve */

	int dev_counter;           /* used to generate device_ids for this client */

	struct list_head urb_list; /* list of all pending urbs of this client */
	struct work_struct unregister_work;
	struct mutex lock;         /* mutex */

	/* shared mem */
	l4dm_dataspace_t *ds;
	ddeusb_urb *d_urbs;
	l4_addr_t d_urbs_phys;
};

struct ddeusb_urb_list_entry {
	struct urb *urb;
	struct list_head list;
};


struct ddeusb_client_urb_entry {
	struct urb *urb;
	struct list_head list;
};

struct ddeusb_client_idlist_entry {
	struct usb_device_id *id;
	struct list_head list;
};


extern l4_threadid_t ddeusb_main_server;

/* COMMON FUNCTIONS */

struct ddeusb_client * ddeusb_get_client_by_task_id(l4_threadid_t t);

#endif
