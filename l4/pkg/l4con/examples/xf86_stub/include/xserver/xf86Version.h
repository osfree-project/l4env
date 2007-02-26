/* $XFree86: xc/programs/Xserver/hw/xfree86/xf86Version.h,v 3.472.2.6 2001/08/09 21:40:47 dawes Exp $ */

#ifndef XF86_VERSION_CURRENT

#define XF86_VERSION_MAJOR	4
#define XF86_VERSION_MINOR	1
#define XF86_VERSION_PATCH	0
#define XF86_VERSION_SNAP	1

/* This has five arguments for compatibilty reasons */
#define XF86_VERSION_NUMERIC(major,minor,patch,snap,dummy) \
	(((major) * 10000000) + ((minor) * 100000) + ((patch) * 1000) + snap)

/* Define these for compatibility.  They'll be removed at some point. */
#define XF86_VERSION_SUBMINOR	XF86_VERSION_PATCH
#define XF86_VERSION_BETA	0
#define XF86_VERSION_ALPHA	XF86_VERSION_SNAP

#define XF86_VERSION_CURRENT					\
   XF86_VERSION_NUMERIC(XF86_VERSION_MAJOR,			\
			XF86_VERSION_MINOR,			\
			XF86_VERSION_PATCH,			\
			XF86_VERSION_SNAP,			\
			0)


#define XF86_DATE	"21 December 2001"

#endif

/* $XConsortium: xf86Version.h /main/78 1996/10/28 05:42:10 kaleb $ */
