struct module_info {
	char	*type;		/* kind of module */
	char	*name;		/* name and version */
	char	*conflict;	/* lowest version to which the module is compatibe with */
	char	*uses;		/* required modules */
};

struct module_struct {
	struct module_info	*info;
	int					(*init) 	(void);
	int					(*deinit) 	(void);
	void				*services;
};
