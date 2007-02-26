#ifndef PRESENTATION
#define PRESENTATION struct public_presentation
#endif

struct presentation_methods;

struct public_presentation {
        struct presenter_methods *gen;
	struct presentation_methods *presm;
};

struct presentation_methods {
	ARRAYLIST 	*(*get_slides_of_pres)		(PRESENTATION *);
 	void    	 (*set_slide_order)        	(PRESENTATION *, int *postions);
        int 		*(*get_slide_order)      	(PRESENTATION *);
	SLIDE		*(*get_slide)			(PRESENTATION *, int key);
 	void		 (*add_slide)            	(PRESENTATION *, SLIDE *sl);
	void	         (*del_slide)			(PRESENTATION *, int key);	
	void 		 (*del_all_slides)		(PRESENTATION *);
	void             (*set_path)			(PRESENTATION *, char *);
	char		*(*get_path)			(PRESENTATION *);
};

struct presentation_services {
        PRESENTATION *(*create)        (void);
};


