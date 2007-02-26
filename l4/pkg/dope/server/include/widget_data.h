/*** widget state flags ***/

#define	WID_FLAGS_STATE		0x0001
#define	WID_FLAGS_FOCUS		0x0002

/*** widget update flags ***/

#define WID_UPDATE_SIZE		0x0001
#define WID_UPDATE_STATE	0x0002
#define	WID_UPDATE_FOCUS	0x0004
#define WID_UPDATE_PARENT	0x0008
#define WID_UPDATE_POS		0x0010

struct binding_struct;
struct binding_struct {
	s16						ev_type;	/* event type */
	char					*bind_ident;/* action event string */
	char					*msg;		/* associated message */
	struct binding_struct	*next;		/* next binding */
};

struct widget_data {
	long	x,y,w,h;				/* current relative position and size */
	long	ox,oy,ow,oh;			/* previous position and size */
	long	min_w,min_h;			/* minimal size */
	long	max_w,max_h;			/* maximal size */
	long	flags;					/* state flags  */
	long	update;					/* update flags */
	void	*context;				/* associated data */
	void	*parent;				/* parent in widget hierarchy */
	WIDGET	*next;					/* next widget in a connected list */
	WIDGET	*prev;					/* previous widget in a connected list */
	void	(*click) (void *);		/* event handle routine */
	long	ref_cnt;				/* reference counter */
	s32	app_id;						/* application that owns the widget */
	struct binding_struct *bindings;/* event bindings */
};
