struct dataprovider_services  {
ARRAYLIST		*(*load_config)		(char *fname);
int			 (*load_content)	(char *fname, l4dm_dataspace_t *ds);
void			 (*wait_for_fprov)	(void);
int			 (*online_check_fprov)	(void);
};

