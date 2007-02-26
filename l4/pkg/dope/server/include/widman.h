struct widman_services {
	void	(*default_widget_data) 		(struct widget_data *);
	void	(*default_widget_methods) 	(struct widget_methods *);
	void	(*build_script_lang)		(void *widtype,struct widget_methods *);
};
