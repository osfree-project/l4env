#if !defined(WIDGET)
#include "widget.h"
#endif

struct redraw_services {
	void  (*draw_area)       (WIDGET *win,s32 x1,s32 y1,s32 x2,s32 y2);
	void  (*draw_widget)     (WIDGET *wid);
	void  (*draw_widgetarea) (WIDGET *wid,s32 x1,s32 y1,s32 x2,s32 y2);
	s32   (*exec_redraw)     (s32 max_pixels);
	u32   (*get_noque)       (void);
	float (*get_avr_ppt)     (void);
	float (*get_min_ppt)     (void);
};
