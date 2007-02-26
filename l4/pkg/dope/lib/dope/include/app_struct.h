
#define EVENT_QUEUE_SIZE 100
#define MAX_DOPE_CLIENTS 8

struct dopelib_app_struct {
	dope_event event_queue[EVENT_QUEUE_SIZE];
	char bindarg_queue[EVENT_QUEUE_SIZE][256];
	long first;
	long last;
	long app_id;
	CORBA_Environment env;
};

extern struct dopelib_app_struct *dopelib_apps[MAX_DOPE_CLIENTS];

