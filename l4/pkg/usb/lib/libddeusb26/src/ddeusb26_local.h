/*
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */



#ifndef L4_USB_LIB_DDEUSB26_LOCAL_H
#define L4_USB_LIB_DDEUSB26_LOCAL_H




#include <l4/dde/linux26/dde26.h> 
#include <l4/log/l4log.h> 
#include <l4/sys/types.h> 
#include <l4/sys/ipc.h> 
#include <l4/util/util.h>
#include <l4/env/errno.h>
#include <l4/lock/lock.h>
#include <l4/sys/types.h>
#include <l4/thread/thread.h>
#include <l4/util/l4_macros.h>
#include <linux/usb.h>
#include <l4/usb/usb_types.h>

#include <l4/usb/libddeusb.h>



#undef WARN_UNIMPL
#ifdef DDEUSB_DEBUG
#define WARN_UNIMPL         printk("unimplemented: %s\n", __FUNCTION__)
#define DEBUG_MSG(msg, ...) printk("%s: \033[36m"msg"\033[0m\n", __FUNCTION__, ##__VA_ARGS__)
#else
#define WARN_UNIMPL
#define DEBUG_MSG(msg, ...)
#endif



#define PORT_C_MASK ((USB_PORT_STAT_C_CONNECTION | USB_PORT_STAT_C_ENABLE | \
        USB_PORT_STAT_C_SUSPEND | USB_PORT_STAT_C_OVERCURRENT | \
        USB_PORT_STAT_C_RESET) << 16)

#define DDEUSB_VHCD_PORT_COUNT 13

#define DDEUSB_VHCD_BUF_SIZE 4069


struct ddeusb_vhcd_gadget 
{
	int id;
	struct usb_device *udev;
	int speed;
	int status;
};


struct ddeusb_vhcd_urbpriv {
	int urb_id;
	int is_dma;
    int status;
#ifdef DDEUSB_MEASURE
    int count;
#endif
    struct urb *urb;
};

struct ddeusb_vhcd_priv_data {
	
	spinlock_t lock;
    
    /* the urbs wich wait for completion from server side */
	struct ddeusb_vhcd_urbpriv *rcv_buf[DDEUSB_VHCD_BUF_SIZE];	
	struct semaphore rcv_buf_free;
    
    /* the urbs wich have received completion and should be given back */
    /* to the core, by the bottom half irq-handler */
    struct urb *give_bakbuf[DDEUSB_VHCD_BUF_SIZE];
	
	struct workqueue_struct * wq; 
	
    u32 port_status[DDEUSB_VHCD_PORT_COUNT];
	struct ddeusb_vhcd_gadget gadget[DDEUSB_VHCD_PORT_COUNT];

};

#endif
