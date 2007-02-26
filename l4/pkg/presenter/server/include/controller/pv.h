struct public_presenter_view {
};

struct presenter_view methods {
	  void            (*show_presentation)    (PRESENTER_VIEW *, PRESENTATION *p);
};

struct presenter_view_services {
	PRESENTER_VIEW	*(*create)		(void);
};

