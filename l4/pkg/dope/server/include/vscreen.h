#if !defined(VSCREEN)
#define VSCREEN struct public_vscreen
#endif

struct vscreen_methods;

struct public_vscreen {
	struct widget_methods 	*gen;
	struct vscreen_methods 	*vscr;
};

struct vscreen_methods {
	void (*reg_server)    (VSCREEN *, u8 *server_ident);
	u8  *(*get_server)    (VSCREEN *);
	void (*set_framerate) (VSCREEN *, s32 fps);
	s32  (*get_framerate) (VSCREEN *);
	s32  (*probe_mode)    (VSCREEN *,s32 width, s32 height, s32 depth);
	s32  (*set_mode)      (VSCREEN *,s32 width, s32 height, s32 depth);
	void (*waitsync)      (VSCREEN *);
	u8  *(*map)           (VSCREEN *, u8 *dst_thread_ident);
	void (*refresh)       (VSCREEN *, s32 x, s32 y, s32 w, s32 h);
};

struct vscreen_services {
	VSCREEN *(*create) (void);
};
