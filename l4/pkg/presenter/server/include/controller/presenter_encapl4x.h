#ifndef PRESENTER_ENCAPL4X
#define PRESENTER_ENCAPL4X struct public_presenter_encapl4x
#endif

struct presenter_encapl4x_methods;

struct public_presenter_encapl4x {
	struct presenter_encapl4x_methods *encapl4xm;
};

struct presenter_encapl4x_methods {
	int	(*put)	(PRESENTER_ENCAPL4X *,char *, l4dm_dataspace_t *);
        int     (*get)  (PRESENTER_ENCAPL4X *, int, l4dm_dataspace_t *);
};

struct presenter_encapl4x_services {
	PRESENTER_ENCAPL4X *(*create)        (void);
};
