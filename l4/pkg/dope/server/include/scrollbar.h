#ifndef SCROLLBAR
#define SCROLLBAR struct public_scrollbar
#endif

#define	SCROLLBAR_HOR 	0
#define SCROLLBAR_VER 	1

struct scrollbar_methods;

struct public_scrollbar {
	struct widget_methods 		*gen;
	struct scrollbar_methods 	*scroll;
};

struct scrollbar_methods {
	void	(*set_type) 		(SCROLLBAR *,u32 type);
	u32	(*get_type) 		(SCROLLBAR *);
	void	(*set_slider_x)		(SCROLLBAR *,s32 new_sx);
	u32	(*get_slider_x)		(SCROLLBAR *);
	void	(*set_slider_y)		(SCROLLBAR *,s32 new_sy);
	u32	(*get_slider_y)		(SCROLLBAR *);
	void	(*set_real_size)	(SCROLLBAR *,u32 new_real_size);
	u32	(*get_real_size)	(SCROLLBAR *);
	void	(*set_view_size)	(SCROLLBAR *,u32 new_view_size);
	u32	(*get_view_size)	(SCROLLBAR *);
	void	(*set_view_offset)	(SCROLLBAR *,s32 new_view_offset);
	u32	(*get_view_offset)	(SCROLLBAR *);
	s32	(*get_arrow_size)	(SCROLLBAR *);
	void	(*reg_scroll_update)(SCROLLBAR *,void (*callback)(void *,u16),void *arg);
};

struct scrollbar_services {
	SCROLLBAR	*(*create)	(void);
};
