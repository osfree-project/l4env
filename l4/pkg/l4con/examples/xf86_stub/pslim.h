#ifndef _PSLIM_H_
#define _PSLIM_H_

#include <l4/sys/types.h>
#include <l4/dm_generic/types.h>

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"

/* All drivers need this */
#include "xf86_ansic.h"

#include "compiler.h"

/* ShadowFB support */
#include "shadow.h"

#include "fb.h"
#include "afb.h"
#include "mfb.h"
#include "cfb24_32.h"

#include "xaa.h"

#define PSLIM_VERSION		4000
#define PSLIM_NAME		"PSLIM"
#define PSLIM_DRIVER_NAME	"pslim"
#define PSLIM_MAJOR_VERSION	1
#define PSLIM_MINOR_VERSION	0
#define PSLIM_PATCHLEVEL	0

typedef struct _PSLIMRec
{
  EntityInfoPtr pEnt;
  GDevPtr device;
  CARD16 maxBytesPerScanline;
  int pix24bpp;
  CARD8 *fbPtr;
  CloseScreenProcPtr CloseScreen;
  XAAInfoRecPtr AccelInfoRec;
  l4_threadid_t vc_tid;
  int dropscon_dev;
  int accel_flags;
  int SavedFgColor;
  Bool shadowFB;
  Bool mapShadow;
  Bool fbMapped;
  OptionInfoPtr Options;
  l4dm_dataspace_t mapShadowDs;
} PSLIMRec, *PSLIMPtr;

typedef struct _ModeInfoData 
{
  int mode;
  int memory_model;
} ModeInfoData;


#endif /* _PSLIM_H_ */
