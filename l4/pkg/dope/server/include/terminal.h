#if !defined(TERMINAL)
#define TERMINAL struct public_terminal
#endif

#if !defined(WIDGETARG)
#define WIDGETARG void
#endif

struct terminal_methods;

struct public_terminal {
	struct widget_methods 	*gen;
	struct terminal_methods *term;
};

struct terminal_methods {
	void	(*print)		(TERMINAL *,char *txt);
};

struct terminal_services {
	TERMINAL	*(*create)	(void);
};
