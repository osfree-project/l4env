/*
 * mostly taken from usbhid package
 *
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */





/* L4 */
#include <l4/util/macros.h>
#include <l4/lock/lock.h>
#include <l4/env/errno.h>
#include <l4/input/libinput.h>
#include <l4/generic_io/libio.h>
/* Linux */
#include <linux/input.h>

#include "internal.h"

#define L4EVDEV_DEVICES 16

#define DEBUG_L4EVDEV 0

/** l4evdev device structure */
struct l4evdev {
	int exists;
	int isopen;
	int devn;
	char name[16];
	struct input_handle handle;
};

/** all known devices */
static struct l4evdev l4evdev_devices[L4EVDEV_DEVICES];
#define DEVS l4evdev_devices

/** HANDLE INCOMING EVENTS (CALLBACK VARIANT) **/
static void(*callback)(struct l4input *) = NULL;

static void l4evdev_event_cb(struct input_handle *handle, unsigned int type,
                             unsigned int code, int value)
{

#if DEBUG_L4EVDEV
	static unsigned long count = 0;
#endif
	static struct l4input ev;

	/* filter sound events */
	if (test_bit(EV_SND, handle->dev->evbit)) return;
	/* filter input_repeat_key() */
	if ((type == EV_KEY) && (value == 2)) return;

	ev.type = type;
	ev.code = code;
	ev.value = value;

	/* call back */

	callback(&ev);

#if DEBUG_L4EVDEV
	LOG_printf("l4evdev.c: Event[%ld]. Dev: %s, Type: %d, Code: %d, Value: %d\n",
	           count++, handle->dev->name, type, code, value);
#endif
//
}

/* XXX had connect/disconnect to be locked? */

static struct input_handle * l4evdev_connect(struct input_handler *handler,
                                             struct input_dev *dev, const struct input_device_id *id)
{
	struct l4evdev *evdev;
	int devn;

	for (devn = 0; (devn < L4EVDEV_DEVICES) && (DEVS[devn].exists); devn++);
	if (devn == L4EVDEV_DEVICES) {
		LOG_printf("l4evdev.c: no more free l4evdev devices\n");
		return NULL;
	}

	evdev = &DEVS[devn];

	memset(evdev, 0, sizeof (struct l4evdev));

	evdev->exists = 1;
	evdev->devn = devn;

	sprintf(evdev->name, "event%d", devn);

	evdev->handle.dev = dev;
	evdev->handle.handler = handler;
	evdev->handle.private = evdev;

	input_open_device(&evdev->handle);

	LOG_printf("connect \"%s\"\n", dev->name);

	return &evdev->handle;
}

static void l4evdev_disconnect(struct input_handle *handle)
{
	struct l4evdev *evdev = handle->private;

	evdev->exists = 0;

	if (evdev->isopen)
		input_close_device(handle);
	else /* XXX what about pending events? */
		memset(&DEVS[evdev->devn], 0, sizeof(struct l4evdev));
}

static struct input_device_id l4evdev_ids[] = {
	{ .driver_info = 1 },  /* Matches all devices */
	{ },                   /* Terminating zero entry */
};

static struct input_handler l4evdev_handler = {
	.event =      l4evdev_event_cb,
	.connect =    l4evdev_connect,
	.disconnect = l4evdev_disconnect,
	.fops =       NULL,               /* not used */
	.minor =      0,                  /* not used */
	.name =       "l4evdev",
	.id_table =   l4evdev_ids
};

int l4input_internal_l4evdev_init(void (*cb)(struct l4input *))
{
	if (!cb) return -EINVAL;

	callback = cb;
	input_register_handler(&l4evdev_handler);

	return 0;
}

void l4input_internal_l4evdev_exit(void)
{
	input_unregister_handler(&l4evdev_handler);
}
