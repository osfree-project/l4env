/*
 * This file lets USB driver subscribe to all device IDs they support...
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#include <linux/usb.h>
#include <l4/usb/libddeusb.h>

#undef usb_deregister

extern void usb_deregister(struct usb_driver *);

/* Theese functions are wrapper for usb register &co. */

int libddeusb26_register_driver(struct usb_driver * driver, struct module * module) {
	
	int i;
	const struct usb_device_id *id = driver->id_table;
	for (; id->idVendor || id->bDeviceClass || id->bInterfaceClass ||
			id->driver_info; id++) {
		libddeusb_subscribe_for_device_id((ddeusb_usb_device_id *)id);
/*		printk("GADGET: %X, %X, %X, %X, %X, %X, %X, %X, %X, %X ,%X\n", id->match_flags, id->idVendor, id->idProduct,id->bcdDevice_lo, id->bcdDevice_hi, id->bDeviceClass, id->bDeviceSubClass, id->bDeviceProtocol, id->bInterfaceClass, id->bInterfaceSubClass, id->bInterfaceProtocol, id->driver_info);
 */
	}
	return usb_register_driver(driver,module);
}

void libdde26_deregister_driver(struct usb_driver* driver) {
	/* TODO: unregisterdriver, do we need it? */
	usb_deregister(driver);
}
