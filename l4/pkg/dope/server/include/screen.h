struct screen_services {
	long  (*set_screen)     (long width, long height, long depth);
	void  (*restore_screen) (void);
	long  (*get_scr_width)  (void);
	long  (*get_scr_height) (void);
	long  (*get_scr_depth)  (void);
	void *(*get_scr_adr)    (void);
	void *(*get_buf_adr)    (void);
	void  (*update_area)    (long x1, long y1, long x2, long y2);
	void  (*set_mouse_pos)  (long x,long y);
	void  (*set_mouse_shape)(void *);
	void  (*mouse_off)      (void);
	void  (*mouse_on)       (void);
	void  (*set_draw_area)	(long x1, long y1, long x2, long y2);
};
