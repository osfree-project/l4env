#if !defined(WIDGET)
#define WIDGET void
#endif

#define USERSTATE_IDLE       0
#define USERSTATE_WINMOVE    1
#define USERSTATE_KEYREPEAT  2
#define USERSTATE_DRAG       3
#define USERSTATE_WINSIZE    4
#define USERSTATE_SCROLLDRAG 5
#define USERSTATE_SCROLLSTEP 6

struct userstate_services {

	void     (*set)             (long new_state,WIDGET *);
	long     (*get)             (void);
	void     (*handle)          (void);
	WIDGET  *(*get_curr_focus)  (void);

};
