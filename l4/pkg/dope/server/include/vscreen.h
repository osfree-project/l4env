/*
 * \brief   Interface of DOpE VScreen widget module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

struct vscreen_methods;
struct vscreen_data;

#define VSCREEN struct vscreen

#if !defined(GFX_CONTAINER)
struct gfx_ds;
#define GFX_CONTAINER struct gfx_ds
#endif


struct vscreen {
	struct widget_methods  *gen;
	struct vscreen_methods *vscr;
	struct widget_data     *wd;
	struct vscreen_data    *vd;
};

struct vscreen_methods {
	void (*reg_server)    (VSCREEN *, u8 *server_ident);
	u8  *(*get_server)    (VSCREEN *);
	void (*set_framerate) (VSCREEN *, s32 fps);
	s32  (*get_framerate) (VSCREEN *);
	void (*set_grabmouse) (VSCREEN *, s32 grab_flag);
	s32  (*get_grabmouse) (VSCREEN *);
	s32  (*probe_mode)    (VSCREEN *,s32 width, s32 height, char *mode);
	s32  (*set_mode)      (VSCREEN *,s32 width, s32 height, char *mode);
	void (*waitsync)      (VSCREEN *);
	u8  *(*map)           (VSCREEN *, u8 *dst_thread_ident);
	void (*refresh)       (VSCREEN *, s32 x, s32 y, s32 w, s32 h);
	GFX_CONTAINER *(*get_image) (VSCREEN *);
};

struct vscreen_services {
	VSCREEN *(*create) (void);
};
