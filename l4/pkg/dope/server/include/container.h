#if !defined(CONTAINER)
#define CONTAINER struct public_container
#endif


struct container_methods;

struct public_container {
	struct widget_methods 		*gen;
	struct container_methods 	*cont;
};

struct container_methods {
	void	 (*add)				(CONTAINER *,WIDGETARG *new_element);
	void	 (*remove)			(CONTAINER *,WIDGETARG *element);
	WIDGET	*(*get_content)		(CONTAINER *);
};

struct container_services {
	CONTAINER	*(*create)	(void);
};
