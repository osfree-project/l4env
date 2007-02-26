#include <l4/dope/dopelib.h>

#ifndef PRESENTER_VIEW
#define PRESENTER_VIEW struct public_view
#endif

struct presenter_view_methods;

struct public_view {
	struct presenter_view_methods *pvm;
};

struct presenter_view_methods {
	void	(*show_presentation)	(PRESENTER_VIEW *, PRESENTATION *);
	void	(*vscr_press_callback)	(dope_event *e, void *arg); 
};

struct presenter_view_services {
	PRESENTER_VIEW	*(*create)		(void);
};

