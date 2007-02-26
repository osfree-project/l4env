#ifndef SLIDE
#define SLIDE struct public_slide
#endif

struct slide_methods;

struct public_slide {
	struct presenter_methods *gen;
	struct slide_methods *sm;
};

struct slide_methods {
	int 		 (*set_content)		(SLIDE *, l4dm_dataspace_t *content);
	char	        *(*get_content) 	(SLIDE *);
	void		 (*del_content)		(SLIDE *);
	l4dm_dataspace_t *(*get_ds)		(SLIDE *);
	int 		 (*get_content_size)	(SLIDE *);
};

struct slide_services {
	SLIDE *(*create) 	(void);
};
