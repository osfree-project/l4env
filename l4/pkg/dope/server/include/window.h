#ifndef WINDOW
#define WINDOW struct public_window
#endif

#define	WIN_BORDERS 1
#define WIN_TITLE 2
#define WIN_CLOSER 4
#define WIN_FULLER 8

struct window_methods;

struct public_window {
	struct widget_methods *gen;
	struct window_methods *win;
};

struct window_methods {
	void *(*get_workarea)  (WINDOW *);
	void  (*set_staytop)   (WINDOW *,s16 staytop_flag);
	s16   (*get_staytop)   (WINDOW *);
	void  (*set_elem_mask) (WINDOW *,s32 elem_mask);
	s32   (*get_elem_mask) (WINDOW *);
	void  (*set_title)     (WINDOW *, char *new_title);
	char *(*get_title)     (WINDOW *);
	void  (*set_state)     (WINDOW *,s32 state);
	void  (*activate)      (WINDOW *);
	void  (*open)          (WINDOW *);
	void  (*close)         (WINDOW *);
	void  (*top)           (WINDOW *);
};

struct window_services {
	WINDOW *(*create) (void);
};
