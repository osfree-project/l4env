#ifndef SLIDE
#define SLIDE struct public_slide
#endif

struct slide_methods;

struct public_slide {
	struct presenter_methods *gen;
	struct slide_methods *sm;
};

struct slide_methods {
	void 		 (*set_content)		(SLIDE *, l4dm_dataspace_t *content);
	l4dm_dataspace_t *(*get_content) 	(SLIDE *);
	l4_addr_t	 (*get_content_addr)	(SLIDE *); 
};

struct slide_services {
	SLIDE *(*create) 	(void);
};
