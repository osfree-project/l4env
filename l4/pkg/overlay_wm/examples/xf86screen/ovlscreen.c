/*
 * \brief   Overlay Screen driver module for XFree86
 * \date    2003-08-21
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This module is derived from the pSLIM driver module by Frank Mehnert.
 * Thanks to Frank for his valuable help!
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
 */

/*** XFREE HEADERS ***/
#include "mipointer.h"      /* for initializing the software cursor */
#include "micmap.h"         /* colormap handling */
#include "xf86cmap.h"
#include "xf86Priv.h"
#include "xf86Bus.h"
#include "xf86.h"           /* all drivers should typically include these */
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86_ansic.h"
#include "compiler.h"
#include "shadow.h"         /* shadow fb support */
#include "fb.h"             /* frame buffer support */
#include "afb.h"
#include "mfb.h"
#include "cfb24_32.h"

#include "ovl_screen.h"

#define OVLSCREEN_VERSION        4000
#define OVLSCREEN_NAME           "OVLSCREEN"
#define OVLSCREEN_DRIVER_NAME    "ovlscreen"
#define OVLSCREEN_MAJOR_VERSION  1
#define OVLSCREEN_MINOR_VERSION  0
#define OVLSCREEN_PATCHLEVEL     0


/*** PRIVATE DRIVER DATA ***/
struct ovlscreen {
	int width;
	int height;
	int depth;
	void *fb_adr;
	CloseScreenProcPtr CloseScreen;
};

#undef ALLOW_OFFSCREEN_PIXMAPS

/*** MANDATORY FUNCTIONS WHICH MUST BE PROVIDED TO THE X-SERVER ***/
static const OptionInfoRec *OVLSCREENAvailableOptions(int chipid, int busid);
static void OVLSCREENIdentify(int flags);
static Bool OVLSCREENProbe(DriverPtr drv, int flags);
static Bool OVLSCREENPreInit(ScrnInfoPtr scrinfo, int flags);
static Bool OVLSCREENScreenInit(int Index, ScreenPtr pScreen, int argc, char **argv);
static Bool OVLSCREENEnterVT(int scrnIndex, int flags);
static void OVLSCREENLeaveVT(int scrnIndex, int flags);
static Bool OVLSCREENCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool OVLSCREENSaveScreen(ScreenPtr pScreen, int mode);
static Bool OVLSCREENSwitchMode(int scrnIndex, DisplayModePtr pMode, int flags);
static void OVLSCREENAdjustFrame(int scrnIndex, int x, int y, int flags);
static void OVLSCREENFreeScreen(int scrnIndex, int flags);
static void OVLSCREENFreeRec(ScrnInfoPtr scrinfo);


/*** DRIVER INFORMATION STRUCTURES ***
 *
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added the driver list by
 * the Module Setup funtion in the dynamic case.  In the static case a
 * reference to this is compiled in, and this requires that the name of
 * this DriverRec be an upper-case version of the driver name.
 */
DriverRec OVLSCREEN = {
	OVLSCREEN_VERSION,
	OVLSCREEN_DRIVER_NAME,
	OVLSCREENIdentify,
	OVLSCREENProbe,
	OVLSCREENAvailableOptions,
	NULL,
	0
};

enum GenericTypes {
	CHIP_OVLSCREEN_GENERIC
};

static SymTabRec OVLSCREENChipsets[] = {
	{CHIP_OVLSCREEN_GENERIC, "ovlscreen"},
	{-1, NULL}
};

static const OptionInfoRec OVLSCREENOptions[] = {
	{-1, NULL, OPTV_NONE, {0}, FALSE}
};


/*** REQUIRED EXTERNAL SYMBOLS ***
 *
 * List of symbols from other modules that this module references.  This
 * list is used to tell the loader that it is OK for symbols here to be
 * unresolved providing that it hasn't been told that they haven't been
 * told that they are essential via a call to xf86LoaderReqSymbols() or
 * xf86LoaderReqSymLists().  The purpose is this is to avoid warnings about
 * unresolved symbols that are not required.
 */
static const char *fbSymbols[] = {
	"fbScreenInit",
	"fbPictureInit",
	"cfb24_32ScreenInit",
	NULL
};

static const char *shadowSymbols[] = {
	"shadowAlloc",
	"shadowInit",
	NULL
};


#ifdef XFree86LOADER

/*** MODULE LOADER INTERFACE ***/
static MODULESETUPPROTO(ovlscreenSetup);

static XF86ModuleVersionInfo ovlscreenVersionRec = {
	OVLSCREEN_DRIVER_NAME,
	"TU Dresden, OS Group, Norman Feske",
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	OVLSCREEN_MAJOR_VERSION, OVLSCREEN_MINOR_VERSION, OVLSCREEN_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,         /* This is a video driver */
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0, 0, 0, 0}
};

/*
 * This data is accessed by the loader.  The name must be the module name
 * followed by "ModuleData".
 */
XF86ModuleData ovlscreenModuleData = { &ovlscreenVersionRec, ovlscreenSetup, NULL };


/*** ENTRY POINT OF THE DRIVER ***/
static pointer ovlscreenSetup(pointer Module, pointer Options,
                              int *ErrorMajor, int *ErrorMinor) {
	static Bool Initialised = FALSE;

	if (!Initialised) {
		Initialised = TRUE;
		xf86AddDriver(&OVLSCREEN, Module, 0);
		LoaderRefSymLists(fbSymbols, shadowSymbols, NULL);
		return (pointer) TRUE;
	}

	if (ErrorMajor)
		*ErrorMajor = LDR_ONCEONLY;
	return (NULL);
}

#endif


/*** UTILITY: LOADING A PALETTE (NOT REALLY) ***
 *
 * This function is provided to the xf86HandleColormaps call.
 */
static void load_palette(ScrnInfoPtr scrinfo, int numColors,
                                 int *indices, LOCO * colors, VisualPtr pVisual) {
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "load palette called\n");
	/* nothing to do */
}


/*** UTILITY: RETURN POINTER TO PRIVATE DRIVER DATA ***
 *
 * If there does not exist a private data structure yet, we
 * create one here.
 */
static struct ovlscreen *get_private_data(ScrnInfoPtr scrinfo) {
	if (!scrinfo->driverPrivate) {
		scrinfo->driverPrivate = xcalloc(sizeof(struct ovlscreen), 1);
	}
	return (struct ovlscreen *)scrinfo->driverPrivate;
}


/*** EXPORT: FREE PRIVATE DRIVER DATA ***/
static void OVLSCREENFreeRec(ScrnInfoPtr scrinfo) {
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "free rec called\n");
	xfree(scrinfo->driverPrivate);
	scrinfo->driverPrivate = NULL;
}


/*** EXPORT: PROVIDE INFORMATION ABOUT DRIVER PARAMETERS ***/
static const OptionInfoRec *OVLSCREENAvailableOptions(int chipid, int busid) {
	return (OVLSCREENOptions);
}


/*** EXPORT: TELL THE PUBLIC WHO WE ARE ***/
static void OVLSCREENIdentify(int flags) {
	xf86PrintChipsets(OVLSCREEN_NAME, "driver for Overlay Screen Server",
	                   OVLSCREENChipsets);
}


/*** EXPORT: PROBE IF THE DRIVER IS ABLE TO RUN ***
 *
 * This function is called once, at the start of the first server generation to
 * do a minimal probe for supported hardware.
 */
static Bool OVLSCREENProbe(DriverPtr drv, int flags) {
	Bool foundScreen = FALSE;
	int numDevSections;
	GDevPtr *devSections;
	int i, entity;
	ScrnInfoPtr scrinfo = NULL;
	EntityInfoPtr pEnt;

	/*
	 * Find the config file Device sections that match this
	 * driver, and return if there are none.
	 */
	numDevSections = xf86MatchDevice(OVLSCREEN_NAME, &devSections);
	if (numDevSections <= 0) return (FALSE);

	for (i = 0; i < numDevSections; i++) {
		entity = xf86ClaimFbSlot(drv, 0, devSections[i], TRUE);

		pEnt = xf86GetEntityInfo(entity);
		scrinfo = xf86AllocateScreen(pEnt->driver, 0);
		xf86AddEntityToScreen(scrinfo, entity);

		scrinfo->driverVersion = OVLSCREEN_VERSION;
		scrinfo->driverName    = OVLSCREEN_DRIVER_NAME;
		scrinfo->name          = OVLSCREEN_NAME;
		scrinfo->Probe         = OVLSCREENProbe;
		scrinfo->PreInit       = OVLSCREENPreInit;
		scrinfo->ScreenInit    = OVLSCREENScreenInit;
		scrinfo->SwitchMode    = OVLSCREENSwitchMode;
		scrinfo->AdjustFrame   = OVLSCREENAdjustFrame;
		scrinfo->EnterVT       = OVLSCREENEnterVT;
		scrinfo->LeaveVT       = OVLSCREENLeaveVT;
		scrinfo->FreeScreen    = OVLSCREENFreeScreen;
		foundScreen = TRUE;
	}

	xfree(devSections);
	return (foundScreen);
}


/*** EXPORT: DRIVER PRE INITIALISATION ***
 *
 * This function is called once for each screen at the start of the first
 * server generation to initialise the screen for all server generations.
 */
static Bool OVLSCREENPreInit(ScrnInfoPtr scrinfo, int flags) {
	struct ovlscreen *ovlscr;
	DisplayModePtr dispmode;
	Gamma gzeros = { 0.0, 0.0, 0.0 };
	rgb rzeros = { 0, 0, 0 };
	int res;

	if (flags & PROBE_DETECT) return (FALSE);

	if ((res = ovl_screen_init(NULL)) < 0) {
		xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG,
		          "Error: ovl_screen_init() returned %d\n", res);
		return (FALSE);
	}

	ovlscr = get_private_data(scrinfo);
	ovlscr->width  = ovl_get_phys_width();  //1024;
	ovlscr->height = ovl_get_phys_height(); //768;
	ovlscr->depth  = ovl_get_phys_mode();

	ovl_screen_open(ovlscr->width, ovlscr->height, ovlscr->depth);

	ovlscr->fb_adr = ovl_screen_map();

	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG,
	            "Overlay Screen is %dx%d@%d\n",
	            ovlscr->width, ovlscr->height, ovlscr->depth);

	/* clear screen */
	if (ovlscr->depth == 16) {
		short *dst = (short *)ovlscr->fb_adr;
		int n = ovlscr->width*ovlscr->height;
		for (;n--;) *(dst++) = 0;
	}
	ovl_screen_refresh(0, 0, ovlscr->width, ovlscr->height);
	
	/* hack: override config entry */
	scrinfo->bitsPerPixel = ovlscr->depth;
	scrinfo->confScreen->defaultdepth = ovlscr->depth;

	scrinfo->chipset = "ovlscreen";
	scrinfo->monitor = scrinfo->confScreen->monitor;
	scrinfo->progClock = TRUE;
	scrinfo->rgbBits = 8;

	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "Overlay Screen: set depth bpp\n");
	if (!xf86SetDepthBpp(scrinfo, 8, 8, 8,
	    (ovlscr->depth == 32) ? Support32bppFb : Support24bppFb)) {
		return (FALSE);
	}
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "Overlay Screen: printdepthbpp\n");
	xf86PrintDepthBpp(scrinfo);

	/* color weight */
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "Overlay Screen: color weight\n");
	if (scrinfo->depth > 8 && !xf86SetWeight(scrinfo, rzeros, rzeros))
		return (FALSE);

	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "Overlay Screen: set default visual\n");
	
	/* visual init */
	if (!xf86SetDefaultVisual(scrinfo, -1)) {
		xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "Overlay Screen: failed\n");

		return (FALSE);
	}
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "Overlay Screen: passed default visual\n");

	xf86SetGamma(scrinfo, gzeros);

	/* XXX */
	scrinfo->videoRam = 4 * 1024 * 1024;

	/* Set display resolution */
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "Overlay Screen: set dpi\n");
	xf86SetDpi(scrinfo, 0, 0);

	scrinfo->monitor->DDC = NULL;

	/* allocate mode */
	dispmode = xcalloc(sizeof(DisplayModeRec), 1);
	dispmode->status = MODE_OK;
	dispmode->type = M_T_BUILTIN;

	/* for adjust frame */
	dispmode->HDisplay = ovlscr->width;
	dispmode->VDisplay = ovlscr->height;

	scrinfo->modePool = dispmode;
	dispmode->next = dispmode->prev = dispmode;

	scrinfo->modes = scrinfo->modePool;
	scrinfo->currentMode = scrinfo->modes;
	scrinfo->displayWidth = ovlscr->width;
	scrinfo->virtualX = ovlscr->width;
	scrinfo->virtualY = ovlscr->height;

	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "Overlay Screen: PrintModes(scrinfo)\n");
	xf86PrintModes(scrinfo);

	scrinfo->bitmapBitOrder = BITMAP_BIT_ORDER;
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "Overlay Screen: reqsym(fbPictureInit)\n");
	xf86LoaderReqSymbols("fbPictureInit", NULL);

	/* load shadow fb module */
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "Using \"Shadow Framebuffer\"\n");
	if (!xf86LoadSubModule(scrinfo, "shadow")) {
		OVLSCREENFreeRec(scrinfo);
		return (FALSE);
	}
	xf86LoaderReqSymLists(shadowSymbols, NULL);

	/* load fb module */
	if (xf86LoadSubModule(scrinfo, "fb") == NULL) {
		OVLSCREENFreeRec(scrinfo);
		return (FALSE);
	}
	xf86LoaderReqSymbols("fbScreenInit", NULL);

	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "finished preinit\n");
	return (TRUE);
}


/*** CALLBACK: UPDATE DIRTY SCREEN REGIONS ***
 *
 * This function is called by the shadow screen module.
 * It forwards the information about the dirty areas to
 * the Overlay Screen server.
 */
#if XF86_VERSION_CURRENT < XF86_VERSION_NUMERIC(4,2,0,0,0)
static void update_screenarea(ScreenPtr pScreen, PixmapPtr pShadow, RegionPtr damage) {
	int nbox = REGION_NUM_RECTS(damage);
	BoxPtr pbox = REGION_RECTS(damage);
#else
static void update_screenarea(ScreenPtr pScreen, shadowBufPtr pBuf) {
	int nbox = REGION_NUM_RECTS(&pBuf->damage);
	BoxPtr pbox = REGION_RECTS(&pBuf->damage);
#endif
	while (nbox--) {
		int rx,ry,rw,rh;
		rx = pbox->x1;
		ry = pbox->y1;
		rw = pbox->x2 - pbox->x1 + 1;
		rh = pbox->y2 - pbox->y1 + 1;
		ovl_screen_refresh(rx,ry,rw,rh);
		pbox++;
	}
}


/*** EXPORT: INITIALIZE SCREEN ***
 *
 * This is the actual function for initializing the frame buffer 
 * and mouse cursor.
 */
static Bool OVLSCREENScreenInit(int scrnIndex, ScreenPtr pScreen,
                                int argc, char **argv) {
	ScrnInfoPtr scrinfo = xf86Screens[scrnIndex];
	struct ovlscreen *ovlscr = get_private_data(scrinfo);
	VisualPtr visual;
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "called screen init\n");

	/* XXX */
	scrinfo->videoRam = 4 * 1024 * 1024;

	/* we are active */
	scrinfo->vtSema = TRUE;

	/* mi layer */
	miClearVisualTypes();
	if (!xf86SetDefaultVisual(scrinfo, -1))
		return (FALSE);
	if (scrinfo->bitsPerPixel > 8) {
		if (!miSetVisualTypes(scrinfo->depth, TrueColorMask,
		      scrinfo->rgbBits, TrueColor))
			return (FALSE);
	} else {
		if (!miSetVisualTypes(scrinfo->depth,
		      miGetDefaultVisualMask(scrinfo->depth),
		      scrinfo->rgbBits, scrinfo->defaultVisual))
			return (FALSE);
	}
	if (!miSetPixmapDepths())
		return (FALSE);

	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "screeninit: fbscreen init\n");
	if (!fbScreenInit(pScreen, ovlscr->fb_adr,
	                  scrinfo->virtualX, scrinfo->virtualY,
	                  scrinfo->xDpi, scrinfo->yDpi,
	                  scrinfo->displayWidth, scrinfo->bitsPerPixel)) {
		return (FALSE);
	}
	fbPictureInit(pScreen, 0, 0);

	if (scrinfo->bitsPerPixel > 8) {
		/* Fixup RGB ordering */
		visual = pScreen->visuals + pScreen->numVisuals;
		while (--visual >= pScreen->visuals) {
			if ((visual->class | DynamicClass) == DirectColor) {
				visual->offsetRed = scrinfo->offset.red;
				visual->offsetGreen = scrinfo->offset.green;
				visual->offsetBlue = scrinfo->offset.blue;
				visual->redMask = scrinfo->mask.red;
				visual->greenMask = scrinfo->mask.green;
				visual->blueMask = scrinfo->mask.blue;
			}
		}
	}

	if (!shadowInit(pScreen, update_screenarea, 0))
		return (FALSE);

	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "screeninit: setblackwhite\n");
	xf86SetBlackWhitePixels(pScreen);
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "screeninit: initbackingstore\n");
	miInitializeBackingStore(pScreen);
	xf86SetBackingStore(pScreen);

	/* software cursor */
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "screeninit: software cursor\n");
	miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

	/* colormap */
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "screeninit: colormap\n");
	if (!miCreateDefColormap(pScreen))
		return (FALSE);

	if (!xf86HandleColormaps(pScreen, 256, 6, load_palette, NULL, 0))
		return (FALSE);

	ovlscr->CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = OVLSCREENCloseScreen;
	pScreen->SaveScreen = OVLSCREENSaveScreen;

	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "screeninit finished\n");
	return (TRUE);
}


/*** EXPORT: ENTER VIRTUAL TERMINAL ***
 *
 * Just a dummy implementation
 */
static Bool OVLSCREENEnterVT(int scrnIndex, int flags) {
	ScrnInfoPtr scrinfo = xf86Screens[scrnIndex];
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "enter vt called\n");
	return (TRUE);
}


/*** EXPORT: LEAVE VIRTUAL TERMINAL ***
 *
 * Just a dummy implementation
 */
static void OVLSCREENLeaveVT(int scrnIndex, int flags) {
	ScrnInfoPtr scrinfo = xf86Screens[scrnIndex];
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "leave vt called\n");
}


/*** EXPORT: CLOSE SCREEN ***
 *
 * A close screen function is called from the DIX layer for each
 * screen at the end of each server generation.
 */
static Bool OVLSCREENCloseScreen(int scrnIndex, ScreenPtr pScreen) {
	ScrnInfoPtr scrinfo = xf86Screens[scrnIndex];
	struct ovlscreen *ovlscr = get_private_data(scrinfo);
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "close screen called\n");

	ovl_screen_close();

	pScreen->CloseScreen = ovlscr->CloseScreen;
	return pScreen->CloseScreen(scrnIndex, pScreen);
}


/*** EXPORT: SWITCH RESOLUTION ***
 *
 * Just a dummy implementation
 */
static Bool OVLSCREENSwitchMode(int scrnIndex, DisplayModePtr pMode, int flags) {
	xf86DrvMsg(scrnIndex, X_CONFIG, "switch mode called\n");
	xf86DrvMsg(scrnIndex, X_CONFIG, "%s\n", __FUNCTION__);
	return (TRUE);
}


/*** EXPORT: SET BEGINNING OF FRAME BUFFER ***
 *
 * Just a dummy implementation
 */
static void OVLSCREENAdjustFrame(int scrnIndex, int x, int y, int flags) {
	xf86DrvMsg(scrnIndex, X_CONFIG, "adjust frame buffer called\n");
	xf86DrvMsg(scrnIndex, X_CONFIG, "%s\n", __FUNCTION__);
}


/*** EXPORT: DEALLOCATE RESOURCES OF A SCREEN ***/
static void OVLSCREENFreeScreen(int scrnIndex, int flags) {
	ScrnInfoPtr scrinfo = xf86Screens[scrnIndex];
	struct ovlscreen *ovlscr = get_private_data(scrinfo);
	xf86DrvMsg(scrnIndex, X_CONFIG, "free screen called\n");

	xf86DrvMsg(scrnIndex, X_CONFIG, "%s\n", __FUNCTION__);
	OVLSCREENFreeRec(xf86Screens[scrnIndex]);
	ovlscr->fb_adr = NULL;
}


/*** EXPORT: SAVE SCREEN ***/
static Bool OVLSCREENSaveScreen(ScreenPtr pScreen, int mode) {
	ScrnInfoPtr scrinfo = xf86Screens[pScreen->myNum];
	Bool on = xf86IsUnblank(mode);
	xf86DrvMsg(scrinfo->scrnIndex, X_CONFIG, "save screen called\n");

	if (on) SetTimeSinceLastInputEvent();

	return (TRUE);
}

