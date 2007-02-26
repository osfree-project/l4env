/*
 * \brief   Linux Input Event driver module for XFree86
 * \date    2004-10-06
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This module is roughly based on the Void example driver module.
 * It processes input events from the Linux Input interface
 * (e.g. /dev/input/event0) and passes them to the X server. It
 * does not distinct between keyboard and mouse devices because
 * an event device can be both. You can use it as mouse and
 * keyboard driver by just executing it from two distinct input
 * sections in your XF86Config file and using different event
 * devices for each section.
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
 */

/*** XFREE INCLUDES ***/
#include <xf86_OSproc.h>
#include <xf86Xinput.h>
#include <exevents.h>         /* needed for initvaluator/proximity */

#ifdef XFree86LOADER
#include <xf86Module.h>
#endif

#define MAXBUTTONS 3
#define MAX_READ_EVENTS 16    /* number of events to read at once */

#define EV_KEY 0x01
#define EV_REL 0x02
#define EV_ABS 0x03

/*************************************
 *** FUNCTION/MACRO KEYS VARIABLES ***
 *************************************/

static KeySym lxevent_map[8] = {
	NoSymbol,  /* 0x00 */
	NoSymbol,  /* 0x01 */
	NoSymbol,  /* 0x02 */
	NoSymbol,  /* 0x03 */
	NoSymbol,  /* 0x04 */
	NoSymbol,  /* 0x05 */
	NoSymbol,  /* 0x06 */
	NoSymbol   /* 0x07 */
};


/*
 * minKeyCode = 8 because this is the min legal key code
 */
static KeySymsRec lxevent_keysyms = {
	lxevent_map, /* map        */
	8,            /* minKeyCode */
	8,            /* maxKC      */
	1             /* width      */
};


static const char *DEFAULTS[] = {
	NULL
};


/*** DRIVER SPECIFIC PRIVATE STRUCTURE ***/
struct private {
	int abs_x;
	int abs_y;
};


/*** CONFIGURE INPUT DEVICE ***
 *
 * called to change the state of a device
 */
static int LXEVENT_device_control(DeviceIntPtr device, int what) {
	unsigned char map[MAXBUTTONS + 1];
	InputInfoPtr pInfo = device->public.devicePrivate;
	int i;

	switch (what) {
	case DEVICE_INIT:
		device->public.on = FALSE;

		for (i = 0; i < MAXBUTTONS; i++) map[i + 1] = i + 1;

		if (!InitButtonClassDeviceStruct(device, MAXBUTTONS, map)) {
			ErrorF("unable to allocate Button class device\n");
			return !Success;
		}

		if (!InitFocusClassDeviceStruct(device)) {
			ErrorF("unable to init Focus class device\n");
			return !Success;
		}

		if (!InitKeyClassDeviceStruct(device, &lxevent_keysyms, NULL)) {
			ErrorF("unable to init key class device\n");
			return !Success;
		}

		if (!InitValuatorClassDeviceStruct(device, 2,
		                                   xf86GetMotionEvents,
		                                   0, Absolute)) {
		
			InitValuatorAxisStruct(device, 0, 0,   /* min val    */
			                                  1,   /* max val    */
			                                  1,   /* resolution */
			                                  0,   /* min_res    */
			                                  1);  /* max_res    */

			InitValuatorAxisStruct(device, 1, 0,   /* min val    */
			                                  1,   /* max val    */
			                                  1,   /* resolution */
			                                  0,   /* min_res    */
			                                  1);  /* max_res    */

			ErrorF("unable to allocate Valuator class device\n");
			return !Success;
		} else {

			/* allocate the motion history buffer if needed */
			xf86MotionHistoryAllocate(pInfo);
		}
		break;

	case DEVICE_ON:
		device->public.on = TRUE;
		break;

	case DEVICE_OFF:
	case DEVICE_CLOSE:
		device->public.on = FALSE;
		break;
	}
	return Success;
}


/*** READ INPUT DATA ***/
static void
LXEVENT_read_input(InputInfoPtr pInfo) {
	struct {
		long long time;
		unsigned short type;
		unsigned short code;
		unsigned int value;
	} ev[MAX_READ_EVENTS]; /* input event read buffer               */
	int len = 0;           /* number of received bytes              */
	int num_events;        /* number of received events             */
	int post_motion = 0;   /* set if motion event need to be posted */
	int i;

	/* read a bunch of events */
	len = read(pInfo->fd, &ev, sizeof(ev));
	if (len < 0) {
		ErrorF("LXEVENT_read_input: read error\n");
		return;
	}

	/* determine number of received events */
	num_events = len / sizeof(ev[0]);

	/* go through event buffer */
	for (i = 0; i < num_events; i++) {

		/* absolute motion */
		if (ev[i].type == EV_ABS) {
			struct private *priv = (struct private *)pInfo->private;

			int x = ev[i].code ? priv->abs_x : ev[i].value;
			int y = ev[i].code ? ev[i].value : priv->abs_y;

			priv->abs_x = x;
			priv->abs_y = y;

			post_motion = 1;
		} else

		/* relative motion */
		if (ev[i].type == EV_REL) {
			struct private *priv = (struct private *)pInfo->private;

			int dx = ev[i].code ? 0 : ev[i].value;
			int dy = ev[i].code ? ev[i].value : 0;

			/*
			 * We track the absolute cursor position in our
			 * private data structure and read it later in
			 * the conversion_proc.
			 */
			priv->abs_x += dx;
			priv->abs_y += dy;

			post_motion = 1;
		} else

		/* key/button */
		if (ev[i].type == EV_KEY) {

			/*
			 * If a key of button is pressed, make sure that we move the
			 * mouse to the current position before evaluating the key.
			 */
			if (post_motion)
				xf86PostMotionEvent(pInfo->dev, 1, 0, 2, 0, 0);
			post_motion = 0;

			/* check for mouse buttons */
			if ((ev[i].code == 272) || (ev[i].code == 273) || (ev[i].code == 274)) {
				xf86PostButtonEvent(pInfo->dev, 0, ev[i].code - 271, ev[i].value, 0, 0);

			/* otherwise generate keyboard event */
			} else {
				xEvent xev;

				/*
				 * Question of the day:
				 * Why dont we use the xf86PostKeyEvent mechanism?
				 */
				xev.u.u.detail = ev[i].code + 8;
				xev.u.u.type   = ev[i].value ? KeyPress : KeyRelease;
				xf86eqEnqueue(&xev);
			}
		}
	}

	/* post pending motion events that occured after the last button/key event */
	if (post_motion)
		xf86PostMotionEvent(pInfo->dev, 1, 0, 2, 0, 0);
}


/*** CONVERT MOTION COORDINATES ***
 *
 * This function is called from within the xf86PostMotionEvent function.
 */
static Bool
LXEVENT_conversion_proc(InputInfoPtr pInfo,
                         int first, int num,
                         int v0, int v1, int v2, int v3, int v4, int v5,
                         int *x, int *y) {
	struct private *priv = (struct private *)pInfo->private;

	/* clamp absolute values to be positive */
	if (priv->abs_x < 0)
		priv->abs_x = 0;
	if (priv->abs_y < 0)
		priv->abs_y = 0;

	/* return absolute value */
	*x = priv->abs_x;
	*y = priv->abs_y;

	return TRUE;
}


/*** DEINIT DRIVER ***
 *
 * called when the driver is unloaded
 */
static void
LXEVENT_deinit(InputDriverPtr drv, InputInfoPtr pInfo, int flags) {
	LXEVENT_device_control(pInfo->dev, DEVICE_OFF);
}


/*** INITIALIZE DRIVER ***
 *
 * called when the module subsection is found in XF86Config
 */
static InputInfoPtr
LXEVENT_init(InputDriverPtr drv, IDevPtr dev, int flags) {
	InputInfoPtr pInfo;
	struct private *priv;

	if (!(pInfo = xf86AllocateInput(drv, 0))) return NULL;
	if (!(priv  = xcalloc(sizeof(struct private), 1))) return NULL;

	/* init the input info record */
	pInfo->name                    = dev->identifier;
	pInfo->type_name               = "LxEvent";
	pInfo->device_control          = LXEVENT_device_control;
	pInfo->read_input              = LXEVENT_read_input;
	pInfo->motion_history_proc     = xf86GetMotionEvents;
	pInfo->history_size            = 0;
	pInfo->control_proc            = NULL;
	pInfo->close_proc              = NULL;
	pInfo->switch_mode             = NULL;
	pInfo->conversion_proc         = LXEVENT_conversion_proc;
	pInfo->reverse_conversion_proc = NULL;
	pInfo->fd                      = -1;
	pInfo->dev                     = NULL;
	pInfo->private_flags           = 0;
	pInfo->always_core_feedback    = 0;
	pInfo->conf_idev               = dev;
	pInfo->private                 = priv;
	pInfo->flags                   = XI86_KEYBOARD_CAPABLE
	                               | XI86_POINTER_CAPABLE
	                               | XI86_SEND_DRAG_EVENTS;

	/* collect the options, and process the common options. */
	xf86CollectInputOptions(pInfo, DEFAULTS, NULL);
	xf86ProcessCommonOptions(pInfo, pInfo->options);

	/* open event device */
	pInfo->fd = xf86OpenSerial(pInfo->options);
	if (pInfo->fd < 0) {
		ErrorF("LXEVENT_init: Error %d while opening device\n", pInfo->fd);
		return NULL;
	}

	/* mark the device configured */
	pInfo->flags |= XI86_CONFIGURED;

	/* make xfree to consider the device for receiving input */
	xf86AddEnabledDevice(pInfo);

	/* return the configured device */
	return (pInfo);
}


#ifdef XFree86LOADER
static
#endif
InputDriverRec LXEVENT = {
	1,                /* driver version */
	"LXEVENT",       /* driver name    */
	NULL,             /* identify       */
	LXEVENT_init,    /* pre-init       */
	LXEVENT_deinit,  /* un-init        */
	NULL,             /* module         */
	0                 /* ref count      */
};


/*********************************
 *** DYNAMIC LOADING FUNCTIONS ***
 *********************************/

#ifdef XFree86LOADER

/*** REMOVE DRIVER ***
 *
 * called when the module subsection is found in XF86Config
 */
static void LXEVENT_unplug(pointer p) { }


/*** ADD DRIVER ***
 *
 * called when the module subsection is found in XF86Config
 */
static pointer
LXEVENT_plug(pointer module, pointer options, int *errmaj, int *errmin) {
	xf86AddInputDriver(&LXEVENT, module, 0);
	return module;
}


static XF86ModuleVersionInfo LXEVENT_version_rec = {
	"lxevent",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	1, 0, 0,
	ABI_CLASS_XINPUT,
	ABI_XINPUT_VERSION,
	MOD_CLASS_XINPUT,
	{0, 0, 0, 0}
};


XF86ModuleData lxeventModuleData = {
	&LXEVENT_version_rec,
	LXEVENT_plug,
	LXEVENT_unplug
};

#endif  /* XFree86LOADER */

