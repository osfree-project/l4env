struct dataprovider_services  {
ARRAYLIST		*(*load_config)	     (char *fname);
void			 (*load_content)     (char *fname, l4dm_dataspace_t *ds);
};

