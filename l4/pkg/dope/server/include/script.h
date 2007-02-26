

struct script_services {
	void	*(*reg_widget_type) 	(char *widtype_name,void *(*create_func)(void));
	void	 (*reg_widget_method)	(void *widtype,char *desc,void *methadr);
	void	 (*reg_widget_attrib)	(void *widtype,char *desc,void *get,void *set,void *update);
	char	*(*exec_command)		(u32 app_id,char *cmd);
};


