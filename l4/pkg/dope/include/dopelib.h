#define EVENT_TYPE_UNDEFINED	0
#define EVENT_TYPE_COMMAND		1
#define EVENT_TYPE_MOTION		2
#define EVENT_TYPE_PRESS		3
#define EVENT_TYPE_RELEASE		4

typedef struct command_event_struct {
	long	type;					/* must be EVENT_TYPE_COMMAND */
	char	*cmd;					/* command string */
} command_event;

typedef struct motion_event_struct {
	long	type;					/* must be EVENT_TYPE_MOTION */
	long	rel_x,rel_y;			/* relative movement in x and y direction */
	long	abs_x,abs_y;			/* current position inside the widget */
} motion_event;

typedef struct press_event_struct {
	long	type;					/* must be EVENT_TYPE_PRESS */
	long	code;					/* code of key/button that is pressed */
} press_event;

typedef struct release_event_struct {
	long	type;					/* must be EVENT_TYPE_RELEASE */
	long	code;					/* code of key/button that is released */
} release_event;

typedef union dopelib_event_union {
	long type;
	command_event	command;
	motion_event	motion;
	press_event		press;
	release_event	release;
} dope_event;


/*** INITIALISE DOpE LIBRARY ***/
extern long  dope_init(void);

/*** DEINITIALISE DOpE LIBRARY ***/
extern void  dope_deinit(void);

/*** REGISTER DOpE CLIENT APPLICATION ***/
extern long  dope_init_app(char *appname);

/*** UNREGISTER DOpE CLIENT APPLICATION ***/
extern long  dope_deinit_app(long app_id);

/*** EXECUTE DOpE COMMAND ***/
extern char *dope_cmd(long app_id,char *command);

/*** EXECUTE DOpE FORMAT STRING COMMAND ***/
extern char *dope_cmdf(long app_id, char *command_format, ...);

/*** BIND AN EVENT TO A DOpE WIDGET ***/
extern void dope_bind(long app_id,char *var,char *event_type,void (*callback)(dope_event *,void *),void *arg);

/*** ENTER DOPE EVENTLOOP ***/
extern void dope_eventloop(long app_id);

/*** REQUEST KEY OR BUTTON STATE ***/
extern long dope_get_keystate(long app_id, long keycode);

/*** REQUEST CURRENT ASCII KEYBOARD STATE ***/
extern char dope_get_ascii(long app_id, long keycode);
