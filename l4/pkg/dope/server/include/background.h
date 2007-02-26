#if !defined(BACKGROUND)
#define BACKGROUND struct public_background
#endif

#define BG_STYLE_WIN 0
#define BG_STYLE_DESK 1

struct background_methods;

struct public_background {
	struct widget_methods 		*gen;
	struct background_methods 	*bg;
};

struct background_methods {
	void	(*set_style)	(BACKGROUND *,long style);
	void	(*set_click)	(BACKGROUND *,void (*click)(void *));
};

struct background_services {
	BACKGROUND	*(*create)	(void);
};
