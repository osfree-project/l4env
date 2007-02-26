#if !defined(WIDGET)
#include "widget.h"
#endif

#if !defined(MUTEX)
#define MUTEX void
#endif

struct rtman_services {
	void (*execute)        (void);
	s32  (*add)            (WIDGET *w,float duration);
	void (*remove)         (WIDGET *w);
	void (*set_sync_mutex) (WIDGET *w,MUTEX *);
};
