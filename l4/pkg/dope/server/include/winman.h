#if !defined(WINDOW)
#include "window.h"
#endif

#if !defined(WIDGET)
#include "widget.h"
#endif

struct winman_services {

	void	 (*add)			(WINDOW *win);
	void	 (*remove)		(WINDOW *win);
	void	 (*draw)		(WINDOW *win,long x1, long y1, long x2, long y2);
	void	 (*draw_behind)	(WINDOW *win,long x1, long y1, long x2, long y2);
	void	 (*activate)	(WINDOW *win);
	void	 (*top)			(WINDOW *win);
	void	 (*move)		(WIDGET *win,long ox,long oy,long nx,long ny);
	WIDGET	*(*find)		(long x,long y);
	void	 (*reorder)		(void);
	void	 (*update_properties) (void);
	
};
