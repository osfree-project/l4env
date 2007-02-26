#ifndef PRESENTER
#define PRESENTER struct public_presenter
#endif

struct presenter_methods;

struct public_presenter {
	struct presenter_methods *gen;
};

/*** FUNCTIONS THAT ALL MODULES HAVE IN COMMON ***/
struct presenter_methods {
	void  (*set_key)		 (PRESENTER *, int key);
	int   (*get_key)		 (PRESENTER *);
	void  (*set_name)		 (PRESENTER *, char *name);
	char *(*get_name)		 (PRESENTER *);
};


struct presenter_data {
	int	key;
        char    *name;
};

struct presenter_general_services {
        void (*default_presenter_methods)       (struct presenter_methods *);
        void (*default_presenter_data)          (struct presenter_data *);
};
