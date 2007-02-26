#if !defined(WIDGET)
#define WIDGET void
#endif

struct input_services {
	long	(*get_mx)				(void);
	long	(*get_my)				(void);
	long	(*get_mb)				(void);
	void	(*set_pos)				(long x,long y);
	long	(*get_keystate)			(long keycode);
	char	(*get_ascii)			(long keycode);
	void	(*update)				(WIDGET *dst);
	void	(*update_properties)	(void);
};
