/* $Id$ */
/*****************************************************************************/
/**
 * \file   input/lib/src/l4evdev.c
 * \brief  L4INPUT Event Device
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * This is roughly a Linux /dev/event implementation adopted from evdev.
 */
/* Original copyright notice from drivers/input/evdev.c follows...
 */
/*
 * Event char devices, giving access to raw input device events.
 *
 * Copyright (c) 1999-2002 Vojtech Pavlik
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

/* L4 */
#include <string.h>
#include <l4/lock/lock.h>
#include <l4/env/errno.h>
#include <l4/input/libinput.h>

/* C */
#include <stdio.h>

/* Linux */
#include <linux/input.h>

#include "internal.h"

/* Okay ...

   There can be up to L4EVDEV_DEVICES managed. 

   So we _don't_ need one event queue/list per device to support dedicated
   event handlers per device -> we use one general event queue (for DOpE) with
   up to L4EVDEV_BUFFER l4evdev_event structures. CON uses callbacks and
   therefore needs no ring buffer.
*/

#define L4EVDEV_DEVICES    16
#define L4EVDEV_BUFFER    512

#define DEBUG_L4EVDEV       0

/** l4evdev event structure */
struct l4evdev_event {
	struct l4evdev *evdev;
	struct input_event event;
};

/** l4evdev device structure */
struct l4evdev {
	int exists;
	int isopen;
	int devn;
	char name[16];
	struct input_handle handle;
};

struct l4evdev *pcspkr;

/** all known devices */
static struct l4evdev l4evdev_devices[L4EVDEV_DEVICES];
/** global event list */
static struct l4evdev_event l4evdev_buffer[L4EVDEV_BUFFER];
static int l4evdev_buffer_head = 0;
static int l4evdev_buffer_tail = 0;
#define DEVS    l4evdev_devices
#define BUFFER  l4evdev_buffer
#define HEAD    l4evdev_buffer_head /**< next event slot */
#define TAIL    l4evdev_buffer_tail /**< first-in event if !(HEAD==TAIL) */
#define INC(x)  (x) = ((x) + 1) % L4EVDEV_BUFFER

static l4lock_t l4evdev_lock = L4LOCK_UNLOCKED_INITIALIZER;


/** HANDLE INCOMING EVENTS (CALLBACK VARIANT) **/
static void(*callback)(struct l4input *) = NULL;

static void l4evdev_event_cb(struct input_handle *handle, unsigned int type,
                             unsigned int code, int value)
{
#if DEBUG_L4EVDEV
	static unsigned long count = 0;
#endif
	static struct l4input ev;

#if DEBUG_L4EVDEV
	count++;
#endif

	if (test_bit(EV_SND, handle->dev->evbit))
		return;

	ev.type = type;
	ev.code = code;
	ev.value = value;

	/* call back */
	callback(&ev);

#if DEBUG_L4EVDEV
	if (!(count % 100))
		printf("l4evdev.c: Event[%ld]. Dev: %s, Type: %d, Code: %d, Value: %d\n",
		       count, handle->dev->phys, type, code, value);
#endif
}

/** HANDLE INCOMING EVENTS (BUFFER AND PULL VARIANT) **/
static void l4evdev_event(struct input_handle *handle, unsigned int type,
                          unsigned int code, int value)
{
#if DEBUG_L4EVDEV
	static unsigned long count = 0;
#endif
	struct l4evdev *evdev = handle->private;

#if DEBUG_L4EVDEV
	count++;
#endif

	if (test_bit(EV_SND, handle->dev->evbit))
		return;

	l4lock_lock(&l4evdev_lock);

	BUFFER[HEAD].evdev = evdev;

	/* XXX no time values in event struct */
	BUFFER[HEAD].event.time.tv_sec = 0;
	BUFFER[HEAD].event.time.tv_usec = 0;

	BUFFER[HEAD].event.type = type;
	BUFFER[HEAD].event.code = code;
	BUFFER[HEAD].event.value = value;

	INC(HEAD);

	/* check head and tail */
	if (HEAD == TAIL) {
		/* clear oldest event struct in buffer */
		//memset(&BUFFER[TAIL], 0, sizeof(struct l4evdev_event));
		INC(TAIL);
	}

	l4lock_unlock(&l4evdev_lock);

#if DEBUG_L4EVDEV
	if (!(count % 100))
		printf("l4evdev.c: Event[%ld]. Dev: %s, Type: %d, Code: %d, Value: %d\n",
		       count, handle->dev->phys, type, code, value);
#endif
}

/* XXX had connect/disconnect to be locked? */

static struct input_handle * l4evdev_connect(struct input_handler *handler,
                                             struct input_dev *dev,
                                             struct input_device_id *id)
{
	struct l4evdev *evdev;
	int devn;

	for (devn = 0; (devn < L4EVDEV_DEVICES) && (DEVS[devn].exists); devn++);
	if (devn == L4EVDEV_DEVICES) {
		printf("l4evdev.c: no more free l4evdev devices\n");
		return NULL;
	}

	evdev = &DEVS[devn];

	memset(evdev, 0, sizeof (struct l4evdev));

	evdev->exists = 1;
	evdev->devn = devn;

	sprintf(evdev->name, "event%d", devn);

	evdev->handle.dev = dev;
	evdev->handle.name = evdev->name;
	evdev->handle.handler = handler;
	evdev->handle.private = evdev;

	input_open_device(&evdev->handle);

	printf("connect \"%s\", %s\n", dev->name, dev->phys);

	if (test_bit(EV_SND, dev->evbit))
		pcspkr = evdev;

	return &evdev->handle;
}

static void l4evdev_disconnect(struct input_handle *handle)
{
	struct l4evdev *evdev = handle->private;

	evdev->exists = 0;

	if (evdev->isopen)
		input_close_device(handle);
		if (test_bit(EV_SND, handle->dev->evbit))
			pcspkr = NULL;
	else /* XXX what about pending events? */
		memset(&DEVS[evdev->devn], 0, sizeof(struct l4evdev));
}

static struct input_device_id l4evdev_ids[] = {
	{ .driver_info = 1 },  /* Matches all devices */
	{ },                   /* Terminating zero entry */
};

static struct input_handler l4evdev_handler = {
	.event =      NULL,               /* fill it on init() */
	.connect =    l4evdev_connect,
	.disconnect = l4evdev_disconnect,
	.fops =       NULL,               /* not used */
	.minor =      0,                  /* not used */
	.name =       "l4evdev",
	.id_table =   l4evdev_ids
};

int l4input_internal_l4evdev_init(void (*cb)(struct l4input *))
{
	if (cb) {
		/* do callbacks */
		callback = cb;
		l4evdev_handler.event = l4evdev_event_cb;
	} else {
		/* buffer events in ring buffer */
		l4evdev_handler.event = l4evdev_event;
	}

	input_register_handler(&l4evdev_handler);

	return 0;
}

void l4input_internal_l4evdev_exit(void)
{
	input_unregister_handler(&l4evdev_handler);
}

/*****************************************************************************/

int l4input_ispending()
{
	return !(HEAD==TAIL);
}

int l4input_flush(void *buffer, int count)
{
	int num = 0;

	l4lock_lock(&l4evdev_lock);

	while ((TAIL!=HEAD) && count) {
		/* flush event buffer */
		/* XXX if sizeof(struct l4input) and sizeof(struct input_event) differ
		   memcpy is not enough  */
		memcpy(buffer, &BUFFER[TAIL].event, sizeof(struct input_event));

		//memset(&BUFFER[TAIL], 0, sizeof(struct l4evdev_event));

		num++; count--;
		INC(TAIL);
		buffer += sizeof(struct input_event);
	}

	l4lock_unlock(&l4evdev_lock);

	return num;
}

int l4input_pcspkr(int tone)
{
	if (!pcspkr)
		return -L4_ENODEV;

	input_event(pcspkr->handle.dev, EV_SND, SND_TONE, tone);

	return 0;
}
