#include <l4/dope/dopelib.h>

#ifndef PRESENTER_VIEW
#define PRESENTER_VIEW struct public_view
#endif

#define DAMAGED_PRESENTATION -2
#define CLOSE_LOAD_DISPLAY -4

struct presenter_view_methods;

struct public_view {
	struct presenter_view_methods *pvm;
};

struct presenter_view_methods {
	void	(*show_presentation)		(PRESENTER_VIEW *, int);
	int 	(*build_view)			(PRESENTER_VIEW *); 
	void 	(*eventloop)			(PRESENTER_VIEW *);
	void    (*update_open_pres)		(PRESENTER_VIEW *);
	void    (*reinit_presenter_window) 	(PRESENTER_VIEW *);
	void	(*init_load_display)		(PRESENTER_VIEW *, int, char *);
	void	(*check_fprov)			(PRESENTER_VIEW *);
	void	(*update_load_display)		(PRESENTER_VIEW *, int);
	int	(*check_slides)			(PRESENTER_VIEW *, int);
	void	(*present_log)			(PRESENTER_VIEW *, char *);
	void	(*present_log_reset)		(PRESENTER_VIEW *);
};

struct presenter_view_services {
	PRESENTER_VIEW	*(*create)		(void);
};

