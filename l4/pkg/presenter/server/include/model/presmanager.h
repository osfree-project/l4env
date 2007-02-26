#ifndef PRESMANAGER
#define PRESMANAGER struct public_presmanager
#endif


struct presmanager_methods;

struct public_presmanager {
        struct presenter_methods *gen;
        struct presmanager_methods *pmm;
};

struct presmanager_methods {
	int		 (*build_presentation)		       (PRESMANAGER *,char *fname);
	void             (*add_presentation)                   (PRESMANAGER *, PRESENTATION *present);
        void             (*del_presentation)                   (PRESMANAGER *, int presentation_key);
	PRESENTATION	*(*get_presentation)		       (PRESMANAGER *, int presenation_key);
	ARRAYLIST       *(*get_presentations)          	       (PRESMANAGER *);
	void		 (*transfer_rights)		       (PRESMANAGER *, int presentation_key);
};

struct presmanager_services {
        PRESMANAGER *(*create)        (void);
};

