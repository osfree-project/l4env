/*
 * Apple USB Touchpad (for post-February 2005 PowerBooks and MacBooks) driver
 *
 * Copyright (C) 2001-2004 Greg Kroah-Hartman (greg@kroah.com)
 * Copyright (C) 2005      Johannes Berg (johannes@sipsolutions.net)
 * Copyright (C) 2005      Stelian Pop (stelian@popies.net)
 * Copyright (C) 2005      Frank Arnold (frank@scirocco-5v-turbo.de)
 * Copyright (C) 2005      Peter Osterlund (petero2@telia.com)
 * Copyright (C) 2005      Michael Hanselmann (linux-kernel@hansmi.ch)
 * Copyright (C) 2006      Nicolas Boichat (nicolas@boichat.ch)
 *
 * Thanks to Alex Harper <basilisk@foobox.net> for his inputs.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>

#ifndef DDE_LINUX

#include <linux/usb/input.h>

#else /* DDE_LINUX */

void * kzalloc(size_t size, int gfp)
{
	void *p = kmalloc(size, gfp);
	if (p) memset(p, 0, size);
	return p;
}

#include <linux/input.h>

static inline void input_sync(struct input_dev *dev)
{
	input_event(dev, EV_SYN, SYN_REPORT, 0);
}

static inline void input_set_abs_params(struct input_dev *dev, int axis, int min, int max, int fuzz, int flat)
{
	dev->absmin[axis] = min;
	dev->absmax[axis] = max;
	dev->absfuzz[axis] = fuzz;
	dev->absflat[axis] = flat;

	dev->absbit[LONG(axis)] |= BIT(axis);
}

struct input_dev *input_allocate_device(void)
{
	struct input_dev *dev;

	dev = kzalloc(sizeof(struct input_dev), GFP_KERNEL);
	if (dev) {
	}

	return dev;
}

void input_free_device(struct input_dev *dev)
{
	if (dev) {
		dev->name = NULL;
		kfree(dev);
	}
}

#include <linux/usb.h>

struct usb_host_endpoint
{
    struct usb_endpoint_descriptor desc;
};

struct usb_host_interface
{
	struct usb_interface_descriptor desc;
};

static inline int usb_kill_urb(struct urb *urb)
{
	return usb_unlink_urb(urb);
}

void *usb_buffer_alloc(struct usb_device *dev, size_t size, int mem_flags,
                       dma_addr_t *dma)
{
    return kmalloc(size, mem_flags);
}

void usb_buffer_free(struct usb_device *dev, size_t size,
                     void *addr, dma_addr_t dma)
{
    kfree(addr);
}

static inline void *usb_get_intfdata (struct usb_interface *intf)
{
	return intf->private_data;
}

static inline void usb_set_intfdata (struct usb_interface *intf, void *data)
{
	intf->private_data = data;
}

#endif /* DDE_LINUX */


/* Apple has powerbooks which have the keyboard with different Product IDs */
#define APPLE_VENDOR_ID		0x05AC

/* These names come from Info.plist in AppleUSBTrackpad.kext */
#define GEYSER_ANSI_PRODUCT_ID	0x0214
#define GEYSER_ISO_PRODUCT_ID	0x0215
#define GEYSER_JIS_PRODUCT_ID	0x0216

/* MacBook devices */
#define GEYSER3_ANSI_PRODUCT_ID	0x0217
#define GEYSER3_ISO_PRODUCT_ID	0x0218
#define GEYSER3_JIS_PRODUCT_ID	0x0219

#define ATP_DEVICE(prod)					\
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE |		\
		       USB_DEVICE_ID_MATCH_INT_CLASS |		\
		       USB_DEVICE_ID_MATCH_INT_PROTOCOL,	\
	.idVendor = APPLE_VENDOR_ID,				\
	.idProduct = (prod),					\
	.bInterfaceClass = 0x03,				\
	.bInterfaceProtocol = 0x02

/* table of devices that work with this driver */
static struct usb_device_id atp_table [] = {
	{ ATP_DEVICE(0x020E) },
	{ ATP_DEVICE(0x020F) },
	{ ATP_DEVICE(0x030A) },
	{ ATP_DEVICE(0x030B) },

	/* PowerBooks Oct 2005 */
	{ ATP_DEVICE(GEYSER_ANSI_PRODUCT_ID) },
	{ ATP_DEVICE(GEYSER_ISO_PRODUCT_ID) },
	{ ATP_DEVICE(GEYSER_JIS_PRODUCT_ID) },

	{ ATP_DEVICE(GEYSER3_ANSI_PRODUCT_ID) },
	{ ATP_DEVICE(GEYSER3_ISO_PRODUCT_ID) },
	{ ATP_DEVICE(GEYSER3_JIS_PRODUCT_ID) },

	/* Terminating entry */
	{ }
};
MODULE_DEVICE_TABLE (usb, atp_table);

/*
 * number of sensors. Note that only 16 instead of 26 X (horizontal)
 * sensors exist on 12" and 15" PowerBooks. All models have 16 Y
 * (vertical) sensors.
 */
#define ATP_XSENSORS	26
#define ATP_YSENSORS	16

/* amount of fuzz this touchpad generates */
#define ATP_FUZZ	16

/* maximum pressure this driver will report */
#define ATP_PRESSURE	300
/*
 * multiplication factor for the X and Y coordinates.
 * We try to keep the touchpad aspect ratio while still doing only simple
 * arithmetics.
 * The factors below give coordinates like:
 *	0 <= x <  960 on 12" and 15" Powerbooks
 *	0 <= x < 1600 on 17" Powerbooks
 *	0 <= y <  646
 */
#define ATP_XFACT	64
#define ATP_YFACT	43

/*
 * Threshold for the touchpad sensors. Any change less than ATP_THRESHOLD is
 * ignored.
 */
#define ATP_THRESHOLD	 5

/* MacBook Pro (Geyser 3) initialization constants */
#define ATP_GEYSER3_MODE_READ_REQUEST_ID 1
#define ATP_GEYSER3_MODE_WRITE_REQUEST_ID 9
#define ATP_GEYSER3_MODE_REQUEST_VALUE 0x300
#define ATP_GEYSER3_MODE_REQUEST_INDEX 0
#define ATP_GEYSER3_MODE_VENDOR_VALUE 0x04

/* Structure to hold all of our device specific stuff */
struct atp {
	char			phys[64];
	struct usb_device *	udev;		/* usb device */
	struct urb *		urb;		/* usb request block */
	signed char *		data;		/* transferred data */
	int			open;		/* non-zero if opened */
	struct input_dev	*input;		/* input dev */
	int			valid;		/* are the sensors valid ? */
	int			x_old;		/* last reported x/y, */
	int			y_old;		/* used for smoothing */
						/* current value of the sensors */
	signed char		xy_cur[ATP_XSENSORS + ATP_YSENSORS];
						/* last value of the sensors */
	signed char		xy_old[ATP_XSENSORS + ATP_YSENSORS];
						/* accumulated sensors */
	int			xy_acc[ATP_XSENSORS + ATP_YSENSORS];
	int			overflowwarn;	/* overflow warning printed? */
	int			datalen;	/* size of an USB urb transfer */
};

#define dbg_dump(msg, tab) \
	if (debug > 1) {						\
		int i;							\
		printk("appletouch: %s %lld", msg, (long long)jiffies); \
		for (i = 0; i < ATP_XSENSORS + ATP_YSENSORS; i++)	\
			printk(" %02x", tab[i]);			\
		printk("\n");						\
	}

#define dprintk(format, a...)						\
	do {								\
		if (debug) printk(format, ##a);				\
	} while (0)

MODULE_AUTHOR("Johannes Berg, Stelian Pop, Frank Arnold, Michael Hanselmann");
MODULE_DESCRIPTION("Apple PowerBooks USB touchpad driver");
MODULE_LICENSE("GPL");

static int debug = 1;
#ifndef DDE_LINUX
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Activate debugging output");
#endif

/* Checks if the device a Geyser 2 (ANSI, ISO, JIS) */
static inline int atp_is_geyser_2(struct atp *dev)
{
	u16 productId = le16_to_cpu(dev->udev->descriptor.idProduct);

	return (productId == GEYSER_ANSI_PRODUCT_ID) ||
		(productId == GEYSER_ISO_PRODUCT_ID) ||
		(productId == GEYSER_JIS_PRODUCT_ID);
}

static inline int atp_is_geyser_3(struct atp *dev)
{
	u16 productId = le16_to_cpu(dev->udev->descriptor.idProduct);

	return (productId == GEYSER3_ANSI_PRODUCT_ID) ||
		(productId == GEYSER3_ISO_PRODUCT_ID) ||
		(productId == GEYSER3_JIS_PRODUCT_ID);
}

static int atp_calculate_abs(int *xy_sensors, int nb_sensors, int fact,
			     int *z, int *fingers)
{
	int i;
	/* values to calculate mean */
	int pcum = 0, psum = 0;

	*fingers = 0;

	for (i = 0; i < nb_sensors; i++) {
		if (xy_sensors[i] < ATP_THRESHOLD)
			continue;
		if ((i - 1 < 0) || (xy_sensors[i - 1] < ATP_THRESHOLD))
			(*fingers)++;
		pcum += xy_sensors[i] * i;
		psum += xy_sensors[i];
	}

	if (psum > 0) {
		*z = psum;
		return pcum * fact / psum;
	}

	return 0;
}

static inline void atp_report_fingers(struct input_dev *input, int fingers)
{
	input_report_key(input, BTN_TOOL_FINGER, fingers == 1);
	input_report_key(input, BTN_TOOL_DOUBLETAP, fingers == 2);
	input_report_key(input, BTN_TOOL_TRIPLETAP, fingers > 2);
}

#ifndef DDE_LINUX
static void atp_complete(struct urb* urb, struct pt_regs* regs)
#else
static void atp_complete(struct urb* urb)
#endif
{
	int x, y, x_z, y_z, x_f, y_f;
	int retval, i, j;
	struct atp *dev = urb->context;

	switch (urb->status) {
	case 0:
		/* success */
		break;
	case -EOVERFLOW:
		if(!dev->overflowwarn) {
			printk("appletouch: OVERFLOW with data "
				"length %d, actual length is %d\n",
				dev->datalen, dev->urb->actual_length);
			dev->overflowwarn = 1;
		}
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		/* This urb is terminated, clean up */
		dbg("%s - urb shutting down with status: %d",
		    __FUNCTION__, urb->status);
		return;
	default:
		dbg("%s - nonzero urb status received: %d",
		    __FUNCTION__, urb->status);
		goto exit;
	}

	/* drop incomplete datasets */
	if (dev->urb->actual_length != dev->datalen) {
		dprintk("appletouch: incomplete data package"
			" (first byte: %d, length: %d).\n",
			dev->data[0], dev->urb->actual_length);
		goto exit;
	}

	/* reorder the sensors values */
	if (atp_is_geyser_3(dev)) {
		memset(dev->xy_cur, 0, sizeof(dev->xy_cur));

		/*
		 * The values are laid out like this:
		 * -, Y1, Y2, -, Y3, Y4, -, ..., -, X1, X2, -, X3, X4, ...
		 * '-' is an unused value.
		 */

		/* read X values */
		for (i = 0, j = 19; i < 20; i += 2, j += 3) {
			dev->xy_cur[i] = dev->data[j + 1];
			dev->xy_cur[i + 1] = dev->data[j + 2];
		}
		/* read Y values */
		for (i = 0, j = 1; i < 9; i += 2, j += 3) {
			dev->xy_cur[ATP_XSENSORS + i] = dev->data[j + 1];
			dev->xy_cur[ATP_XSENSORS + i + 1] = dev->data[j + 2];
		}
	} else if (atp_is_geyser_2(dev)) {
		memset(dev->xy_cur, 0, sizeof(dev->xy_cur));

		/*
		 * The values are laid out like this:
		 * Y1, Y2, -, Y3, Y4, -, ..., X1, X2, -, X3, X4, -, ...
		 * '-' is an unused value.
		 */

		/* read X values */
		for (i = 0, j = 19; i < 20; i += 2, j += 3) {
			dev->xy_cur[i] = dev->data[j];
			dev->xy_cur[i + 1] = dev->data[j + 1];
		}

		/* read Y values */
		for (i = 0, j = 1; i < 9; i += 2, j += 3) {
			dev->xy_cur[ATP_XSENSORS + i] = dev->data[j];
			dev->xy_cur[ATP_XSENSORS + i + 1] = dev->data[j + 1];
		}
	} else {
		for (i = 0; i < 8; i++) {
			/* X values */
			dev->xy_cur[i     ] = dev->data[5 * i +  2];
			dev->xy_cur[i +  8] = dev->data[5 * i +  4];
			dev->xy_cur[i + 16] = dev->data[5 * i + 42];
			if (i < 2)
				dev->xy_cur[i + 24] = dev->data[5 * i + 44];

			/* Y values */
			dev->xy_cur[i + 26] = dev->data[5 * i +  1];
			dev->xy_cur[i + 34] = dev->data[5 * i +  3];
		}
	}

	dbg_dump("sample", dev->xy_cur);

	if (!dev->valid) {
		/* first sample */
		dev->valid = 1;
		dev->x_old = dev->y_old = -1;
		memcpy(dev->xy_old, dev->xy_cur, sizeof(dev->xy_old));

		if (atp_is_geyser_3(dev)) /* No 17" Macbooks (yet) */
			goto exit;

		/* 17" Powerbooks have extra X sensors */
		for (i = (atp_is_geyser_2(dev)?15:16); i < ATP_XSENSORS; i++) {
			if (!dev->xy_cur[i]) continue;

			printk("appletouch: 17\" model detected.\n");
			if(atp_is_geyser_2(dev))
				input_set_abs_params(dev->input, ABS_X, 0,
						     (20 - 1) *
						     ATP_XFACT - 1,
						     ATP_FUZZ, 0);
			else
				input_set_abs_params(dev->input, ABS_X, 0,
						     (ATP_XSENSORS - 1) *
						     ATP_XFACT - 1,
						     ATP_FUZZ, 0);

			break;
		}

		goto exit;
	}

	for (i = 0; i < ATP_XSENSORS + ATP_YSENSORS; i++) {
		/* accumulate the change */
		signed char change = dev->xy_old[i] - dev->xy_cur[i];
		dev->xy_acc[i] -= change;

		/* prevent down drifting */
		if (dev->xy_acc[i] < 0)
			dev->xy_acc[i] = 0;
	}

	memcpy(dev->xy_old, dev->xy_cur, sizeof(dev->xy_old));

	dbg_dump("accumulator", dev->xy_acc);

	x = atp_calculate_abs(dev->xy_acc, ATP_XSENSORS,
			      ATP_XFACT, &x_z, &x_f);
	y = atp_calculate_abs(dev->xy_acc + ATP_XSENSORS, ATP_YSENSORS,
			      ATP_YFACT, &y_z, &y_f);

	if (x && y) {
		if (dev->x_old != -1) {
			x = (dev->x_old * 3 + x) >> 2;
			y = (dev->y_old * 3 + y) >> 2;
			dev->x_old = x;
			dev->y_old = y;

			if (debug > 1)
				printk("appletouch: X: %3d Y: %3d "
				       "Xz: %3d Yz: %3d\n",
				       x, y, x_z, y_z);

			input_report_key(dev->input, BTN_TOUCH, 1);
			input_report_abs(dev->input, ABS_X, x);
			input_report_abs(dev->input, ABS_Y, y);
			input_report_abs(dev->input, ABS_PRESSURE,
					 min(ATP_PRESSURE, x_z + y_z));
			atp_report_fingers(dev->input, max(x_f, y_f));
		}
		dev->x_old = x;
		dev->y_old = y;
	}
	else if (!x && !y) {

		dev->x_old = dev->y_old = -1;
		input_report_key(dev->input, BTN_TOUCH, 0);
		input_report_abs(dev->input, ABS_PRESSURE, 0);
		atp_report_fingers(dev->input, 0);

		/* reset the accumulator on release */
		memset(dev->xy_acc, 0, sizeof(dev->xy_acc));
	}

	input_report_key(dev->input, BTN_LEFT,
			 !!dev->data[dev->datalen - 1]);

	input_sync(dev->input);

exit:
#ifndef DDE_LINUX
	retval = usb_submit_urb(dev->urb, GFP_ATOMIC);
#else
	retval = usb_submit_urb(dev->urb);
#endif
	if (retval) {
		err("%s - usb_submit_urb failed with result %d",
		    __FUNCTION__, retval);
	}
}

static int atp_open(struct input_dev *input)
{
	struct atp *dev = input->private;

#ifndef DDE_LINUX
	if (usb_submit_urb(dev->urb, GFP_ATOMIC))
#else
	if (usb_submit_urb(dev->urb))
#endif
		return -EIO;

	dev->open = 1;
	return 0;
}

static void atp_close(struct input_dev *input)
{
	struct atp *dev = input->private;

	usb_kill_urb(dev->urb);
	dev->open = 0;
}

#ifndef DDE_LINUX
static int atp_probe(struct usb_interface *iface, const struct usb_device_id *id)
#else
static int atp_probe(struct usb_device *udev, unsigned ifnum, const struct usb_device_id *id)
#endif
{
	struct atp *dev;
	struct input_dev *input_dev;
#ifndef DDE_LINUX
	struct usb_device *udev = interface_to_usbdev(iface);
#else
	struct usb_interface *iface = usb_ifnum_to_if(udev, ifnum);
	struct usb_host_interface iface_desc_buffer;
#endif
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int int_in_endpointAddr = 0;
	int i, retval = -ENOMEM;


	/* set up the endpoint information */
	/* use only the first interrupt-in endpoint */
#ifndef DDE_LINUX
	iface_desc = iface->cur_altsetting;
#else
	iface_desc_buffer.desc = iface->altsetting[iface->act_altsetting];
	iface_desc = &iface_desc_buffer;
#endif
	for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
#ifndef DDE_LINUX
		endpoint = &iface_desc->endpoint[i].desc;
#else
		endpoint = usb_epnum_to_ep_desc(udev, i);
#endif
		if (!int_in_endpointAddr &&
		    (endpoint->bEndpointAddress & USB_DIR_IN) &&
		    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
					== USB_ENDPOINT_XFER_INT)) {
			/* we found an interrupt in endpoint */
			int_in_endpointAddr = endpoint->bEndpointAddress;
			break;
		}
	}
	if (!int_in_endpointAddr) {
		err("Could not find int-in endpoint");
		return -EIO;
	}

	/* allocate memory for our device state and initialize it */
	dev = kzalloc(sizeof(struct atp), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!dev || !input_dev) {
		err("Out of memory");
		goto err_free_devs;
	}

	dev->udev = udev;
	dev->input = input_dev;
	dev->overflowwarn = 0;
	if (atp_is_geyser_3(dev))
		dev->datalen = 64;
	else if (atp_is_geyser_2(dev))
		dev->datalen = 64;
	else
		dev->datalen = 81;

	if (atp_is_geyser_3(dev)) {
		/*
		 * By default Geyser 3 device sends standard USB HID mouse
		 * packets (Report ID 2). This code changes device mode, so it
		 * sends raw sensor reports (Report ID 5).
		 */
		char data[8];
		int size;

		size = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			ATP_GEYSER3_MODE_READ_REQUEST_ID,
			USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			ATP_GEYSER3_MODE_REQUEST_VALUE,
			ATP_GEYSER3_MODE_REQUEST_INDEX, &data, 8, 5000);

		if (size != 8) {
			err("Could not do mode read request from device"
							" (Geyser 3 mode)");
			goto err_free_devs;
		}

		/* Apply the mode switch */
		data[0] = ATP_GEYSER3_MODE_VENDOR_VALUE;

		size = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			ATP_GEYSER3_MODE_WRITE_REQUEST_ID,
			USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			ATP_GEYSER3_MODE_REQUEST_VALUE,
			ATP_GEYSER3_MODE_REQUEST_INDEX, &data, 8, 5000);

		if (size != 8) {
			err("Could not do mode write request to device"
							" (Geyser 3 mode)");
			goto err_free_devs;
		}
		printk("appletouch Geyser 3 inited.\n");
	}

#ifndef DDE_LINUX
	dev->urb = usb_alloc_urb(0, GFP_KERNEL);
#else
	dev->urb = usb_alloc_urb(0);
#endif
	if (!dev->urb) {
		retval = -ENOMEM;
		goto err_free_devs;
	}

	dev->data = usb_buffer_alloc(dev->udev, dev->datalen, GFP_KERNEL,
				     &dev->urb->transfer_dma);
	if (!dev->data) {
		retval = -ENOMEM;
		goto err_free_urb;
	}

	usb_fill_int_urb(dev->urb, udev,
			 usb_rcvintpipe(udev, int_in_endpointAddr),
			 dev->data, dev->datalen, atp_complete, dev, 1);

	usb_make_path(udev, dev->phys, sizeof(dev->phys));
#ifndef DDE_LINUX
	strlcat(dev->phys, "/input0", sizeof(dev->phys));
#else
	strncat(dev->phys, "/input0", sizeof(dev->phys));
#endif

	input_dev->name = "appletouch";
#ifndef DDE_LINUX
	input_dev->phys = dev->phys;
	usb_to_input_id(dev->udev, &input_dev->id);
	input_dev->cdev.dev = &iface->dev;
#endif

	input_dev->private = dev;
	input_dev->open = atp_open;
	input_dev->close = atp_close;

	set_bit(EV_ABS, input_dev->evbit);

	if (atp_is_geyser_3(dev)) {
		/*
		 * MacBook have 20 X sensors, 10 Y sensors
		 */
		input_set_abs_params(input_dev, ABS_X, 0,
				     ((20 - 1) * ATP_XFACT) - 1, ATP_FUZZ, 0);
		input_set_abs_params(input_dev, ABS_Y, 0,
				     ((10 - 1) * ATP_YFACT) - 1, ATP_FUZZ, 0);
	} else if (atp_is_geyser_2(dev)) {
		/*
		 * Oct 2005 15" PowerBooks have 15 X sensors, 17" are detected
		 * later.
		 */
		input_set_abs_params(input_dev, ABS_X, 0,
				     ((15 - 1) * ATP_XFACT) - 1, ATP_FUZZ, 0);
		input_set_abs_params(input_dev, ABS_Y, 0,
				     ((9 - 1) * ATP_YFACT) - 1, ATP_FUZZ, 0);
	} else {
		/*
		 * 12" and 15" Powerbooks only have 16 x sensors,
		 * 17" models are detected later.
		 */
		input_set_abs_params(input_dev, ABS_X, 0,
				     (16 - 1) * ATP_XFACT - 1, ATP_FUZZ, 0);
		input_set_abs_params(input_dev, ABS_Y, 0,
				     (ATP_YSENSORS - 1) * ATP_YFACT - 1, ATP_FUZZ, 0);
	}
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, ATP_PRESSURE, 0, 0);

	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, input_dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, input_dev->keybit);
	set_bit(BTN_LEFT, input_dev->keybit);

	input_register_device(dev->input);

	/* save our data pointer in this interface device */
	usb_set_intfdata(iface, dev);

	return 0;

 err_free_urb:
	usb_free_urb(dev->urb);
 err_free_devs:
	usb_set_intfdata(iface, NULL);
	kfree(dev);
	input_free_device(input_dev);
	return retval;
}

static void atp_disconnect(struct usb_interface *iface)
{
	struct atp *dev = usb_get_intfdata(iface);

	usb_set_intfdata(iface, NULL);
	if (dev) {
		usb_kill_urb(dev->urb);
		input_unregister_device(dev->input);
		usb_buffer_free(dev->udev, dev->datalen,
				dev->data, dev->urb->transfer_dma);
		usb_free_urb(dev->urb);
		kfree(dev);
	}
	printk(KERN_INFO "input: appletouch disconnected\n");
}

#ifndef DDE_LINUX
static int atp_suspend(struct usb_interface *iface, pm_message_t message)
{
	struct atp *dev = usb_get_intfdata(iface);
	usb_kill_urb(dev->urb);
	dev->valid = 0;
	return 0;
}

static int atp_resume(struct usb_interface *iface)
{
	struct atp *dev = usb_get_intfdata(iface);
	if (dev->open && usb_submit_urb(dev->urb, GFP_ATOMIC))
		return -EIO;

	return 0;
}
#endif

#ifdef DDE_LINUX
static void * atp_probe_wrapper(struct usb_device *udev, unsigned ifnum, const struct usb_device_id *id)
{
	if (!atp_probe(udev, ifnum, id))
		return (void *)1;
	else
		return 0;
}

static void __attribute__((used)) atp_disconnect_wrapper(struct usb_interface *iface, void *p)
{
	/* FIXME */
	atp_disconnect(iface);
}
#endif

static struct usb_driver atp_driver = {
	.name		= "appletouch",
	.probe		= atp_probe_wrapper,
	.disconnect	= 0 /*atp_disconnect_wrapper*/,
#ifndef DDE_LINUX
	.suspend	= atp_suspend,
	.resume		= atp_resume,
#endif
	.id_table	= atp_table,
};

static int __init atp_init(void)
{
	return usb_register(&atp_driver);
}

static void __exit atp_exit(void)
{
	usb_deregister(&atp_driver);
}

module_init(atp_init);
module_exit(atp_exit);
