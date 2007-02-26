/*!
 * \file	con/examples/xf86_stub/pslim.c
 * \brief	pSLIM interface to console
 *
 * \date	01/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include "pslim.h"

#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/types.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/con/l4con.h>
#include <l4/con/con-client.h>
#include <l4/names/libnames.h>

#include "ioctls.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

/* Colormap handling */
#include "micmap.h"
#include "xf86cmap.h"

#include "xf86Priv.h"
#include "xf86Bus.h"

#undef ALLOW_OFFSCREEN_PIXMAPS

/* Mandatory functions */
static const OptionInfoRec *PSLIMAvailableOptions (int chipid, int busid);
static void PSLIMIdentify (int flags);
static Bool PSLIMProbe (DriverPtr drv, int flags);
static Bool PSLIMPreInit (ScrnInfoPtr pScrn, int flags);
static Bool PSLIMScreenInit (int Index, ScreenPtr pScreen, 
			     int argc, char **argv);
static Bool PSLIMEnterVT (int scrnIndex, int flags);
static void PSLIMLeaveVT (int scrnIndex, int flags);
static Bool PSLIMCloseScreen (int scrnIndex, ScreenPtr pScreen);
static Bool PSLIMSaveScreen (ScreenPtr pScreen, int mode);

static Bool PSLIMSwitchMode (int scrnIndex, DisplayModePtr pMode, int flags);
static void PSLIMAdjustFrame (int scrnIndex, int x, int y, int flags);
static void PSLIMFreeScreen (int scrnIndex, int flags);
static void PSLIMFreeRec (ScrnInfoPtr pScrn);

/* locally used functions */
static void PSLIMLoadPalette (ScrnInfoPtr pScrn, int numColors, int *indices,
			      LOCO *colors, VisualPtr pVisual);

#if XF86_VERSION_CURRENT < XF86_VERSION_NUMERIC(4,2,0,0,0)
static void PSLIMUpdate(ScreenPtr pScreen, PixmapPtr pShadow, RegionPtr damage);
static void PSLIMMapShadowUpdate(ScreenPtr pScreen, PixmapPtr pShadow, 
				 RegionPtr damage);
#else
static void PSLIMUpdate(ScreenPtr pScreen, shadowBufPtr pBuf);
static void PSLIMMapShadowUpdate(ScreenPtr pScreen, shadowBufPtr pBuf);
#endif
static void PSLIMInitAccel(ScreenPtr pScreen);
static Bool PSLIMInitSIGIOHandler(ScrnInfoPtr pScrn);

static Bool PSLIMMapFB (ScrnInfoPtr pScrn);
static void PSLIMUnmapFB (ScrnInfoPtr pScrn);

/* 
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added the driver list by
 * the Module Setup funtion in the dynamic case.  In the static case a
 * reference to this is compiled in, and this requires that the name of
 * this DriverRec be an upper-case version of the driver name.
 */
DriverRec PSLIM = {
  PSLIM_VERSION,
  PSLIM_DRIVER_NAME,
  PSLIMIdentify,
  PSLIMProbe,
  PSLIMAvailableOptions,
  NULL,
  0
};

enum GenericTypes
{
  CHIP_PSLIM_GENERIC
};

/* Supported chipsets */
static SymTabRec PSLIMChipsets[] = {
  {CHIP_PSLIM_GENERIC, "pslim"},
  {-1,                 NULL}
};

typedef enum {
  OPTION_SHADOW_FB,
  OPTION_MAP_SHADOW,
} PSLIMOpts;

static const OptionInfoRec PSLIMOptions[] = {
    { OPTION_SHADOW_FB,  "ShadowFB",    OPTV_BOOLEAN,   {0},    FALSE },
    { OPTION_MAP_SHADOW, "MapShadow",   OPTV_BOOLEAN,   {0},    FALSE },
    { -1,                 NULL,         OPTV_NONE,      {0},    FALSE }
};

/*
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

static const char *xaaSymbols[] = {
  "XAAInit",
  "XAACreateInfoRec",
  NULL
};


#ifdef XFree86LOADER

/* Module loader interface */
static MODULESETUPPROTO (pslimSetup);

static XF86ModuleVersionInfo pslimVersionRec = {
  PSLIM_DRIVER_NAME,
  "TU Dresden, OS Group, Frank Mehnert",
  MODINFOSTRING1,
  MODINFOSTRING2,
  XF86_VERSION_CURRENT,
  PSLIM_MAJOR_VERSION, PSLIM_MINOR_VERSION, PSLIM_PATCHLEVEL,
  ABI_CLASS_VIDEODRV,		/* This is a video driver */
  ABI_VIDEODRV_VERSION,
  MOD_CLASS_VIDEODRV,
  {0, 0, 0, 0}
};

/*
 * This data is accessed by the loader.  The name must be the module name
 * followed by "ModuleData".
 */
XF86ModuleData pslimModuleData = { &pslimVersionRec, pslimSetup, NULL };

static pointer
pslimSetup (pointer Module, pointer Options, int *ErrorMajor, int *ErrorMinor)
{
  static Bool Initialised = FALSE;

  if (!Initialised)
    {
      Initialised = TRUE;
      xf86AddDriver (&PSLIM, Module, 0);
      LoaderRefSymLists(fbSymbols,
			shadowSymbols,
			xaaSymbols,
			NULL);
      return (pointer) TRUE;
    }

  if (ErrorMajor)
    *ErrorMajor = LDR_ONCEONLY;
  return (NULL);
}

#endif

static const OptionInfoRec *
PSLIMAvailableOptions (int chipid, int busid)
{
  return (PSLIMOptions);
}

static void
PSLIMIdentify (int flags)
{
  xf86PrintChipsets (PSLIM_NAME, "driver for DROPS pSLIM protocol", 
		     PSLIMChipsets);
}

/*
 * This function is called once, at the start of the first server generation to
 * do a minimal probe for supported hardware.
 */
static Bool
PSLIMProbe (DriverPtr drv, int flags)
{
  Bool foundScreen = FALSE;
  int numDevSections;
  GDevPtr *devSections;
  int i, entity;
  ScrnInfoPtr pScrn = NULL;
  EntityInfoPtr pEnt;

  /*
   * Find the config file Device sections that match this
   * driver, and return if there are none.
   */
  if ((numDevSections = xf86MatchDevice (PSLIM_NAME, &devSections)) <= 0)
    return (FALSE);

  for (i = 0; i < numDevSections; i++)
    {
      entity = xf86ClaimFbSlot(drv, 0, devSections[i], TRUE);

      pEnt = xf86GetEntityInfo(entity);
      pScrn = xf86AllocateScreen(pEnt->driver, 0);
      xf86AddEntityToScreen(pScrn, entity);
      
      pScrn->driverVersion = PSLIM_VERSION;
      pScrn->driverName    = PSLIM_DRIVER_NAME;
      pScrn->name          = PSLIM_NAME;
      pScrn->Probe         = PSLIMProbe;
      pScrn->PreInit       = PSLIMPreInit;
      pScrn->ScreenInit    = PSLIMScreenInit;
      pScrn->SwitchMode    = PSLIMSwitchMode;
      pScrn->AdjustFrame   = PSLIMAdjustFrame;
      pScrn->EnterVT       = PSLIMEnterVT;
      pScrn->LeaveVT       = PSLIMLeaveVT;
      pScrn->FreeScreen    = PSLIMFreeScreen;
      foundScreen = TRUE;
    }

  xfree (devSections);

  return (foundScreen);
}

static PSLIMPtr
PSLIMGetRec (ScrnInfoPtr pScrn)
{
  PSLIMPtr pSlim = (PSLIMPtr) pScrn->driverPrivate;
  
  if (!pScrn->driverPrivate)
    {
      pScrn->driverPrivate = xcalloc (sizeof (PSLIMRec), 1);
      pSlim = (PSLIMPtr) pScrn->driverPrivate;
      pSlim->mapShadowDs = L4DM_INVALID_DATASPACE;
    }

  return pSlim;
}

static void
PSLIMFreeRec (ScrnInfoPtr pScrn)
{
  xfree (pScrn->driverPrivate);
  pScrn->driverPrivate = NULL;
}

/*
 * This function is called once for each screen at the start of the first
 * server generation to initialise the screen for all server generations.
 */
static Bool
PSLIMPreInit (ScrnInfoPtr pScrn, int flags)
{
  PSLIMPtr pSlim;
  DisplayModePtr pMode;
  ModeInfoData *data = NULL;
  char *mod = NULL;
  const char *reqSym = NULL;
  Gamma gzeros = { 0.0, 0.0, 0.0 };
  rgb rzeros = { 0, 0, 0 };
  int error;
  CORBA_Environment _env = dice_default_environment;
  l4_uint8_t gmode;
  unsigned bits_per_pixel, bytes_per_pixel, bytes_per_line;
  unsigned xres, yres, fn_x, fn_y;
  unsigned sbuf1_size, sbuf2_size, sbuf3_size;

  if (flags & PROBE_DETECT)
    return (FALSE);

  pSlim = PSLIMGetRec (pScrn);
  pSlim->pEnt = xf86GetEntityInfo (pScrn->entityList[0]);
  pSlim->device = xf86GetDevFromEntity (pScrn->entityList[0],
					pScrn->entityInstanceList[0]);

  if ((pSlim->dropscon_dev = open("/proc/dropscon", O_RDONLY)) < 0)
    {
      xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
		  "Error opening dropscon device\n");
      return FALSE;
    }

  if ((error = ioctl(pSlim->dropscon_dev, 
		     _IO(L4IOTYPE, 1), &pSlim->vc_tid)) < 0)
    {
      xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
		  "Error %d ioctl 1 on dropscon device\n", error);
      return FALSE;
    }

  xf86DrvMsg (pScrn->scrnIndex, X_CONFIG,
	      "virtual DROPS console at %x.%x\n", 
	      pSlim->vc_tid.id.task, pSlim->vc_tid.id.lthread);

  /* get maximum receive buffer size which was set in dropscon module */
  if (con_vc_gmode_call(&(pSlim->vc_tid), 
		   &gmode, &sbuf1_size, &sbuf2_size, &sbuf3_size, &_env)
      || (_env.major != CORBA_NO_EXCEPTION))
    {
      xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
		  "Error determining sbuf_size from console parameters "
		  "(exc=%d, %08x)\n", _env.major, _env.ipc_error);
      return (FALSE);
    }

  /* get graphics screen resolution, bpp and Bpp */
  if (con_vc_graph_gmode_call(&(pSlim->vc_tid), &gmode, &xres, &yres, 
			&bits_per_pixel, &bytes_per_pixel,
			&bytes_per_line, &pSlim->accel_flags, 
			&fn_x, &fn_y, &_env)
      || (_env.major != CORBA_NO_EXCEPTION))
    {
      xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
		  "Error determining console parameters (exc=%d, %08x)\n", 
		  _env.major, _env.ipc_error);
      return (FALSE);
    }
  
  xf86DrvMsg (pScrn->scrnIndex, X_CONFIG,
	      "DROPS console is %dx%d@%d (%dBpp, sbuf_size=%d)\n",
	      xres, yres, bits_per_pixel, bytes_per_pixel, sbuf1_size);

  if (sbuf1_size < xres * bytes_per_pixel)
    {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
	      "sbuf1_size at console too small (should be %d but is %d\n",
	      xres*bytes_per_pixel, sbuf1_size);
      return (FALSE);
    }

  /* hack: override config entry */
  pScrn->bitsPerPixel = bits_per_pixel;
  pScrn->confScreen->defaultdepth
    = (bits_per_pixel == 32) ? 24 : bits_per_pixel;
  
  pScrn->chipset = "pslim";
  pScrn->monitor = pScrn->confScreen->monitor;
  pScrn->progClock = TRUE;
  pScrn->rgbBits = 8;

  
  if (!xf86SetDepthBpp (pScrn, 8, 8, 8, 
	(bits_per_pixel == 32) ? Support32bppFb : Support24bppFb))
    return (FALSE);
  xf86PrintDepthBpp (pScrn);

  /* Get the depth24 pixmap format */
  if (pScrn->depth == 24 && pSlim->pix24bpp == 0)
    pSlim->pix24bpp = xf86GetBppFromDepth (pScrn, 24);

  /* color weight */
  if (pScrn->depth > 8 && !xf86SetWeight (pScrn, rzeros, rzeros))
    return (FALSE);

  /* visual init */
  if (!xf86SetDefaultVisual (pScrn, -1))
    return (FALSE);

  xf86SetGamma (pScrn, gzeros);

  /* XXX */
  pScrn->videoRam = 4*1024*1024;

  /* Set display resolution */
  xf86SetDpi (pScrn, 0, 0);

  pScrn->monitor->DDC = NULL;

  /* allocate mode */
  pMode = xcalloc(sizeof(DisplayModeRec), 1);

  pMode->status = MODE_OK;
  pMode->type = M_T_BUILTIN;

  /* for adjust frame */
  pMode->HDisplay = xres;
  pMode->VDisplay = yres;

  data = xcalloc(sizeof(ModeInfoData), 1);
  data->mode = 0; /* only one mode */
  data->memory_model = 6; /* Packed Pixel, XXX get from console server */
  pMode->PrivSize = sizeof(ModeInfoData);
  pMode->Private = (INT32*)data;

  pScrn->modePool = pMode;
  pMode->next = pMode->prev = pMode;

  pScrn->modes = pScrn->modePool;
  pSlim->maxBytesPerScanline = xres * bytes_per_pixel;

  pScrn->currentMode = pScrn->modes;
  pScrn->displayWidth = xres;
  pScrn->virtualX = xres;
  pScrn->virtualY = yres;
  
  xf86PrintModes (pScrn);
  
  /* options */
  xf86CollectOptions(pScrn, NULL);
  if (!(pSlim->Options = xalloc(sizeof(PSLIMOptions))))
    return (FALSE);

  memcpy(pSlim->Options, PSLIMOptions, sizeof(PSLIMOptions));
  xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, pSlim->Options);

  if (xf86ReturnOptValBool(pSlim->Options, OPTION_SHADOW_FB, FALSE))
    pSlim->shadowFB = TRUE;
  
  if (xf86ReturnOptValBool(pSlim->Options, OPTION_MAP_SHADOW, FALSE))
    {
      pSlim->shadowFB = TRUE;
      pSlim->mapShadow = TRUE;
    }
  
  if (0) /* mode not supported */
    {
      xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
		  "Unsupported mode\n");
      return (FALSE);
    }
  else
    {
      mod = "fb";
      reqSym = "fbScreenInit";
      pScrn->bitmapBitOrder = BITMAP_BIT_ORDER;
      xf86LoaderReqSymbols ("fbPictureInit", NULL);

      switch (pScrn->bitsPerPixel)
	{
	case 16:
	case 32:
	  break;
	case 24:
	  if (pSlim->pix24bpp == 32)
	    {
	      mod = "xf24_32bpp";
	      reqSym = "cfb24_32ScreenInit";
	    }
	  break;
	default:
	  xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
		      "Unsupported bpp: %d", pScrn->bitsPerPixel);
	  return FALSE;
	}
    }

  if (pSlim->shadowFB)
    {
      /* load shadowFB  module */
      xf86DrvMsg (pScrn->scrnIndex, X_CONFIG,
		 "Using \"Shadow Framebuffer\"\n");
      if (!xf86LoadSubModule (pScrn, "shadow"))
	{
	  PSLIMFreeRec (pScrn);
	  return (FALSE);
	}

      xf86LoaderReqSymLists (shadowSymbols, NULL);
    }
  else
    {
      /* load accel module */
      if (!xf86LoadSubModule (pScrn, "xaa"))
	{
	  PSLIMFreeRec (pScrn);
	  return (FALSE);
	}
      
      xf86LoaderReqSymLists (xaaSymbols, NULL);
    }
  
  /* load fb module */
  if (mod && xf86LoadSubModule (pScrn, mod) == NULL)
    {
      PSLIMFreeRec (pScrn);
      return (FALSE);
    }
  xf86LoaderReqSymbols (reqSym, NULL);

  if (!PSLIMInitSIGIOHandler (pScrn))
    return (FALSE);
  
  return (TRUE);
}

static Bool
PSLIMMapFB (ScrnInfoPtr pScrn)
{
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);
  static char fb_buf[2*4*1024*1024];
  unsigned map_addr;
  l4_uint32_t offset;
  l4_snd_fpage_t snd_fpage;
  CORBA_Environment _env = dice_default_environment;

  map_addr = (unsigned)(fb_buf+L4_SUPERPAGESIZE-1)&L4_SUPERPAGEMASK;

  /* prevent dangling L4 pages */
  l4_fpage_unmap(l4_fpage(map_addr, L4_LOG2_SUPERPAGESIZE,
		          L4_FPAGE_RW, L4_FPAGE_MAP),
		 L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);
 
  _env.rcv_fpage = l4_fpage(map_addr, L4_LOG2_SUPERPAGESIZE, 0, 0);
  /* map framebuffer from con server */
  if (con_vc_graph_mapfb_call(&(pSlim->vc_tid),
			 &snd_fpage, &offset, &_env)
      || (_env.major != CORBA_NO_EXCEPTION))
    {
      xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
		  "Error mapping framebuffer (exc=%d, %08x)", 
		  _env.major, _env.ipc_error);
      return (FALSE);
    }

  pSlim->fbPtr = (CARD8*)(map_addr + offset);
  pSlim->fbMapped = TRUE;
      
  xf86DrvMsg (pScrn->scrnIndex, X_CONFIG, 
	      "Framebuffer mapped to %08x\n", pSlim->fbPtr);

  return (TRUE);
}

static void
PSLIMUnmapFB (ScrnInfoPtr pScrn)
{
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);

  /* unmap framebuffer */
  l4_fpage_unmap(l4_fpage((unsigned)pSlim->fbPtr, L4_LOG2_SUPERPAGESIZE, 0, 0),
			  L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);
}

static Bool
PSLIMScreenInit (int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
  ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);
  VisualPtr visual;
  int memory_model;

  /* XXX */
  pScrn->videoRam = 4*1024*1024;

  if (pSlim->shadowFB)
    {
      /* use shadow framebuffer */
      if (pSlim->mapShadow)
	{
	  /* we want to map the shadow frame buffer into console so
	   * allocate a dataspace */
	  int error;
	  l4_threadid_t dm_id;
	  l4_size_t size;
	  l4dm_dataspace_t ds;
	  unsigned bpp;
	  CORBA_Environment _env = dice_default_environment;

	  /* where is our default memory dataspace manager */
	  if (!names_waitfor_name(L4DM_MEMPHYS_NAME, &dm_id, 2000))
	    {
	      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			 L4DM_MEMPHYS_NAME " not found");
	      return (FALSE);
	    }
	  
	  bpp  = (pScrn->bitsPerPixel+1) / 8;
	  size = pScrn->virtualX * bpp * pScrn->virtualY;
	  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;
	  
	  /* allocate dataspace */
	  if ((error = l4dm_mem_open(dm_id, size, 0, 0, "xf86 vfb", &ds)))
	    {
	      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			 "Can't allocate dataspace of %dkB\n",
			 size / 1024);
	      return (FALSE);
	    }

	  if ((error = l4rm_attach(&ds, size, 0, 0, (void**)&pSlim->fbPtr)))
	    {
	      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			 "Can't attach data_ds\n");
	      return (FALSE);
	    }

	  /* con_tid has to be a client of the dataspace */
	  if ((error = l4dm_share(&ds, pSlim->vc_tid, L4DM_RO)))
	    {
	      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			 "Can't share data_ds with console\n");
	      return (FALSE);
	    }

	  if (con_vc_direct_setfb_call(&(pSlim->vc_tid), &ds, &_env)
	      || (_env.major != CORBA_NO_EXCEPTION))
	    {
	      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			 "Can't set framebuffer at console (exc=%d, %08x)\n", 
			 _env.major, _env.ipc_error);
	      return (FALSE);
	    }
	  
	  pSlim->mapShadowDs = ds;
	}
      else
	{
	  /* default way like XFree86 does it */
	  if ((pSlim->fbPtr = shadowAlloc (pScrn->virtualX, pScrn->virtualY,
					   pScrn->bitsPerPixel)) == NULL)
	    {
	      xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
			 "Error allocating shadow buffer of size %d\n",
			 pScrn->virtualX*pScrn->virtualY*pScrn->bitsPerPixel/8);
	      return (FALSE);
	    }
	}
    }
  else
    {
      /* use direct mapped framebuffer */
      if (PSLIMMapFB(pScrn) != TRUE)
	return FALSE;
    }

  /* we are active */
  pScrn->vtSema = TRUE;

  /* mi layer */
  miClearVisualTypes ();
  if (!xf86SetDefaultVisual (pScrn, -1))
    return (FALSE);
  if (pScrn->bitsPerPixel > 8)
    {
      if (!miSetVisualTypes (pScrn->depth, TrueColorMask,
			     pScrn->rgbBits, TrueColor))
	return (FALSE);
    }
  else
    {
      if (!miSetVisualTypes (pScrn->depth,
			     miGetDefaultVisualMask (pScrn->depth),
			     pScrn->rgbBits, pScrn->defaultVisual))
	return (FALSE);
    }
  if (!miSetPixmapDepths ())
    return (FALSE);
  
  memory_model = ((ModeInfoData*)pScrn->modes->Private)->memory_model;
  
  switch (memory_model)
    {
    case 0x0:			/* Text mode */
    case 0x1:			/* CGA graphics */
    case 0x2:			/* Hercules graphics */
    case 0x3:			/* Planar */
    case 0x5:			/* Non-chain 4, 256 color */
    case 0x7:			/* YUV */
      xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
		  "Unsupported Memory Model: %d", memory_model);
      return (FALSE);
    case 0x4:			/* Packed pixel */
    case 0x6:			/* Direct Color */
      switch (pScrn->bitsPerPixel)
	{
	case 24:
	  if (pSlim->pix24bpp == 32)
	    {
	      if (!cfb24_32ScreenInit (pScreen,
				       pSlim->fbPtr,
				       pScrn->virtualX, pScrn->virtualY,
				       pScrn->xDpi, pScrn->yDpi,
				       pScrn->displayWidth))
		return (FALSE);
	      break;
	    }
	case 8:
	case 16:
	case 32:
	  if (!fbScreenInit (pScreen,
			     pSlim->fbPtr,
			     pScrn->virtualX, pScrn->virtualY,
			     pScrn->xDpi, pScrn->yDpi,
			     pScrn->displayWidth, pScrn->bitsPerPixel))
	    return (FALSE);
	  fbPictureInit (pScreen, 0, 0);
	  break;
	default:
	  xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
		      "Unsupported bpp: %d", pScrn->bitsPerPixel);
	  return (FALSE);
	}
      break;
    }
  
  if (pScrn->bitsPerPixel > 8)
    {
      /* Fixup RGB ordering */
      visual = pScreen->visuals + pScreen->numVisuals;
      while (--visual >= pScreen->visuals)
	{
	  if ((visual->class | DynamicClass) == DirectColor)
	    {
	      visual->offsetRed   = pScrn->offset.red;
	      visual->offsetGreen = pScrn->offset.green;
	      visual->offsetBlue  = pScrn->offset.blue;
	      visual->redMask     = pScrn->mask.red;
	      visual->greenMask   = pScrn->mask.green;
	      visual->blueMask    = pScrn->mask.blue;
	    }
	}
    }
      
  if (pSlim->shadowFB)
    {
      if (pSlim->mapShadow)
	{
	  if (!shadowInit (pScreen, PSLIMMapShadowUpdate, 0))
	    return (FALSE);
	}
      else
	{
	  if (!shadowInit (pScreen, PSLIMUpdate, 0))
	    return (FALSE);
	}
    }
  else
    {
      PSLIMInitAccel(pScreen);
    }

  xf86SetBlackWhitePixels (pScreen);
  miInitializeBackingStore (pScreen);
  xf86SetBackingStore (pScreen);

  /* software cursor */
  miDCInitialize (pScreen, xf86GetPointerScreenFuncs ());

  /* colormap */
  if (!miCreateDefColormap (pScreen))
    return (FALSE);

  if (!xf86HandleColormaps (pScreen, 256, 6,
			    PSLIMLoadPalette, NULL, 0))
    return (FALSE);

  pSlim->CloseScreen = pScreen->CloseScreen;
  pScreen->CloseScreen = PSLIMCloseScreen;
  pScreen->SaveScreen = PSLIMSaveScreen;

  return (TRUE);
}

static Bool
PSLIMEnterVT (int scrnIndex, int flags)
{
  ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);

//  xf86DrvMsg (scrnIndex, X_CONFIG, "%s\n", __FUNCTION__);

  /* tell the L4Linux stub that we are entering the X mode */
  ioctl(pSlim->dropscon_dev, _IO(L4IOTYPE, 4), NULL);
  
  return (TRUE);
}

static void
PSLIMLeaveVT (int scrnIndex, int flags)
{
  ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);
  
//  xf86DrvMsg (scrnIndex, X_CONFIG, "%s\n", __FUNCTION__);

  /* tell the L4Linux stub that we are leaving the X mode */
  ioctl(pSlim->dropscon_dev, _IO(L4IOTYPE, 3), NULL);
}

/* A close screen function is called from the DIX layer for each
 * screen at the end of each server generation. */                         
static Bool
PSLIMCloseScreen (int scrnIndex, ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);

  if (pSlim->AccelInfoRec)
    {
      xfree (pSlim->AccelInfoRec);
      pSlim->AccelInfoRec = NULL;
    }
  if (pSlim->mapShadow && !l4dm_is_invalid_ds(pSlim->mapShadowDs))
    {
      l4dm_close(&pSlim->mapShadowDs);
      pSlim->mapShadowDs = L4DM_INVALID_DATASPACE;
      pSlim->fbPtr = NULL;
    }
  if (pSlim->fbMapped)
    {
      PSLIMUnmapFB (pScrn);
      pSlim->fbMapped = FALSE;
    }
  if (pSlim->shadowFB && pSlim->fbPtr)
    {
      xfree (pSlim->fbPtr);
      pSlim->fbPtr = NULL;
    }
  
  pScrn->vtSema = FALSE;

  pScreen->CloseScreen = pSlim->CloseScreen;
  return pScreen->CloseScreen (scrnIndex, pScreen);
}

static Bool
PSLIMSwitchMode (int scrnIndex, DisplayModePtr pMode, int flags)
{
  xf86DrvMsg (scrnIndex, X_CONFIG, "%s\n", __FUNCTION__);
  return (TRUE);
}

/* set beginning of frame buffer */
static void
PSLIMAdjustFrame (int scrnIndex, int x, int y, int flags)
{
  xf86DrvMsg (scrnIndex, X_CONFIG, "%s\n", __FUNCTION__);
}

static void
PSLIMFreeScreen (int scrnIndex, int flags)
{
  ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);
  
  xf86DrvMsg (scrnIndex, X_CONFIG, "%s\n", __FUNCTION__);
  PSLIMFreeRec (xf86Screens[scrnIndex]);
  close(pSlim->dropscon_dev);
  
  if (pSlim->shadowFB && pSlim->fbPtr)
    {
      xfree (pSlim->fbPtr);
      pSlim->fbPtr = NULL;
    }
  if (pSlim->mapShadow && !l4dm_is_invalid_ds(pSlim->mapShadowDs))
    {
      l4dm_close(&pSlim->mapShadowDs);
      pSlim->mapShadowDs = L4DM_INVALID_DATASPACE;
    }
  if (pSlim->fbMapped)
    {
      PSLIMUnmapFB (pScrn);
      pSlim->fbMapped = FALSE;
    }
}

static void
#if XF86_VERSION_CURRENT < XF86_VERSION_NUMERIC(4,2,0,0,0)
PSLIMUpdate(ScreenPtr pScreen, PixmapPtr pShadow, RegionPtr damage)
#else
PSLIMUpdate(ScreenPtr pScreen, shadowBufPtr pBuf)
#endif
{
  FbBits *shaBase;
  int shaBpp;
#if XF86_VERSION_CURRENT < XF86_VERSION_NUMERIC(4,2,0,0,0)
  int nbox = REGION_NUM_RECTS (damage);
  BoxPtr pbox = REGION_RECTS (damage);
#else
  int nbox = REGION_NUM_RECTS (&pBuf->damage);
  BoxPtr pbox = REGION_RECTS (&pBuf->damage);
  int shaXoff, shaYoff;
#endif
  FbStride shaStride;
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);

#if XF86_VERSION_CURRENT < XF86_VERSION_NUMERIC(4,2,0,0,0)
  fbGetDrawable (&pShadow->drawable, shaBase, shaStride, shaBpp);
#else
  fbGetDrawable (&pBuf->pPixmap->drawable, shaBase, shaStride, shaBpp,
		 shaXoff, shaYoff);
#endif
  
  while (nbox--)
    {
      l4con_pslim_rect_t rect;
      unsigned stride_width;
      unsigned h, vfbofs;

      rect.x = pbox->x1;
      rect.y = pbox->y1;
      rect.w = pbox->x2 - pbox->x1;
      rect.h = 1;
      h      = pbox->y2 - pbox->y1;
      vfbofs = (unsigned)shaBase + rect.x*(shaBpp/8);
      
      stride_width = rect.w * (shaBpp/8);

      while (h--)
	{
	  CORBA_Environment _env = dice_default_environment;
	 
	  if (con_vc_pslim_set_call(&(pSlim->vc_tid), &rect, 
		(l4_uint8_t*)(vfbofs + rect.y*pSlim->maxBytesPerScanline), 
		stride_width, &_env)
	      || (_env.major != CORBA_NO_EXCEPTION))
	    {
	      xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
			 "Error updating region at console (exc=%d, %08x)\n",
			 _env.major, _env.ipc_error);
	      return;
	    }
	  rect.y++;
	}
      pbox++;
    }
}

static void 
#if XF86_VERSION_CURRENT < XF86_VERSION_NUMERIC(4,2,0,0,0)
PSLIMMapShadowUpdate(ScreenPtr pScreen, PixmapPtr pShadow, RegionPtr damage)
#else
PSLIMMapShadowUpdate(ScreenPtr pScreen, shadowBufPtr pBuf)
#endif
{
  FbBits *shaBase;
  int shaBpp;
#if XF86_VERSION_CURRENT < XF86_VERSION_NUMERIC(4,2,0,0,0)
  int nbox = REGION_NUM_RECTS (damage);
  BoxPtr pbox = REGION_RECTS (damage);
#else
  int nbox = REGION_NUM_RECTS (&pBuf->damage);
  BoxPtr pbox = REGION_RECTS (&pBuf->damage);
  int shaXoff, shaYoff;
#endif
  FbStride shaStride;
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);

#if XF86_VERSION_CURRENT < XF86_VERSION_NUMERIC(4,2,0,0,0)
  fbGetDrawable (&pShadow->drawable, shaBase, shaStride, shaBpp);
#else
  fbGetDrawable (&pBuf->pPixmap->drawable, shaBase, shaStride, shaBpp,
		 shaXoff, shaYoff);
#endif
  
  while (nbox--)
    {
      l4con_pslim_rect_t rect;
      CORBA_Environment _env = dice_default_environment;

      rect.x = pbox->x1;
      rect.y = pbox->y1;
      rect.w = pbox->x2 - pbox->x1;
      rect.h = pbox->y2 - pbox->y1;
      
      /* try to send more than one line */
      if (con_vc_direct_update_call(&(pSlim->vc_tid), &rect, &_env)
	  || (_env.major != CORBA_NO_EXCEPTION))
	{
	  xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
	 	     "Error updating region at console (exc=%d, %08x)\n", 
		     _env.major, _env.ipc_error);
	  return;
	}
      pbox++;
    }
}

static void
PSLIMLoadPalette (ScrnInfoPtr pScrn, int numColors, int *indices,
		 LOCO * colors, VisualPtr pVisual)
{
  /* nothing to do */
}

static Bool
PSLIMSaveScreen(ScreenPtr pScreen, int mode)
{
  Bool on = xf86IsUnblank(mode);

  if (on)
    SetTimeSinceLastInputEvent();

  return (TRUE);
}

static void
PSLIMXaaSetupCopy(ScrnInfoPtr pScrn, int xdir, int ydir, int rop,  
	   unsigned planemask, int transparency_color)
{
}

static void
PSLIMXaaCopy(ScrnInfoPtr pScrn,
	     int x1, int y1, int x2, int y2, int w, int h)
{
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);
  l4con_pslim_rect_t rect = { x1, y1, w, h };
  CORBA_Environment _env = dice_default_environment;

  if (!w || !h)
    return;

  if (con_vc_pslim_copy_call(&(pSlim->vc_tid), &rect, x2, y2, &_env)
      || (_env.major != CORBA_NO_EXCEPTION))
    xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
		"Error copying region at console (exc=%d, %08x)\n", 
		_env.major, _env.ipc_error);
}

static void
PSLIMXaaSetupFill(ScrnInfoPtr pScrn, int color, int rop, unsigned planemask)
{
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);

  /* set highest bit of color to inhibit color converting in console */
  pSlim->SavedFgColor = color | 0x80000000;
}

static void
PSLIMXaaFill(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);
  l4con_pslim_rect_t rect = { x, y, w, h };
  CORBA_Environment _env = dice_default_environment;
  
  if (!w || !h)
    return;

  if (con_vc_pslim_fill_call(&(pSlim->vc_tid), &rect,
			(l4con_pslim_color_t)pSlim->SavedFgColor,
			&_env)
      || (_env.major != CORBA_NO_EXCEPTION))
    xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
		"Error filling region at console (exc=%d, %08x)\n", 
		_env.major, _env.ipc_error);
}

static void
PSLIMXaaSync(ScrnInfoPtr pScrn)
{
  /* sync is not needed since it is done by the console */
}

static void
PSLIMInitAccel(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);
  XAAInfoRecPtr xaaptr;
#ifdef ALLOW_OFFSCREEN_PIXEMAP
  BoxRec AvailFBArea;
#endif

  if (pSlim->accel_flags & L4CON_FAST_COPY)
    {
      if (!(xaaptr = pSlim->AccelInfoRec = XAACreateInfoRec()))
	return;

      /* general flags */
      xaaptr->Flags                        = LINEAR_FRAMEBUFFER
#ifdef ALLOW_OFFSCREEN_PIXMAPS
					   | OFFSCREEN_PIXMAPS
					   | PIXMAP_CACHE
#endif
					   ;

      /* fast copy function */
      xaaptr->SetupForScreenToScreenCopy   = PSLIMXaaSetupCopy;
      xaaptr->SubsequentScreenToScreenCopy = PSLIMXaaCopy;
      xaaptr->ScreenToScreenCopyFlags      = NO_TRANSPARENCY
					   | NO_PLANEMASK
					   | GXCOPY_ONLY
					   ;

      /* fast fill function */
      xaaptr->SetupForSolidFill            = PSLIMXaaSetupFill;
      xaaptr->SubsequentSolidFillRect      = PSLIMXaaFill;
      xaaptr->SolidFillFlags               = NO_PLANEMASK ;
  
      /* wait till graphics engine is idle before direct access to fb */
      xaaptr->Sync                         = PSLIMXaaSync;

#ifdef ALLOW_OFFSCREEN_PIXMAPS
      /* Finally, we set up the video memory space available to the pixmap
       * cache. In this case, all memory from the end of the virtual screen
       * to the end of the command overflow buffer can be used. If you haven't
       * enabled the PIXMAP_CACHE flag, then these lines can be omitted. */
      AvailFBArea.x1 = 0;
      AvailFBArea.y1 = 0;
      AvailFBArea.x2 = pScrn->virtualX;
      AvailFBArea.y2 = 4*1024*1024/pSlim->maxBytesPerScanline;
      xf86InitFBManager(pScreen, &AvailFBArea);
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		 "Using %d lines for offscreen memory.\n",
		 AvailFBArea.y2 - pScrn->virtualY);
#endif
      
      XAAInit(pScreen, xaaptr);
    }
}

/* install SIGIO handler */
static void
PSLIMSIGIOHandler (int fd, void *closure)
{
  int error;
  int arg;
  ScrnInfoPtr pScrn = (ScrnInfoPtr) closure;
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);
  
  if ((error = ioctl(pSlim->dropscon_dev, _IO(L4IOTYPE, 5), &arg)) < 0)
    {
      xf86DrvMsg (0, X_ERROR,
		  "Error %d ioctl 5 on dropscon device\n", error);
      return;
    }

  switch (arg)
    {
    case 1:
      if (!pSlim->shadowFB)
	{
	  PSLIMMapFB (pScrn);
	}
      pScrn->vtSema = TRUE;
      break;
    case 2:
      if (!pSlim->shadowFB)
	{
	  PSLIMUnmapFB (pScrn);
	  pSlim->fbMapped = FALSE;
	}
      pScrn->vtSema = FALSE;
      break;
    }
}

static Bool
PSLIMInitSIGIOHandler(ScrnInfoPtr pScrn)
{
  PSLIMPtr pSlim = PSLIMGetRec (pScrn);

  if (!xf86InstallSIGIOHandler (pSlim->dropscon_dev,
				PSLIMSIGIOHandler, (void*)pScrn))
    {
      xf86DrvMsg(0, X_CONFIG, "Error connecting Sigio");
      return (FALSE);
    }

  return (TRUE);
}

