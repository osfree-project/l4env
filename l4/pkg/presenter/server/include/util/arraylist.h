#ifndef ARRAYLIST
#define ARRAYLIST void 
#endif

struct arraylist_services  {
	ARRAYLIST *(*create) (void);
        void     (*destroy)  (ARRAYLIST *al);
	void     (*add_elem)    (ARRAYLIST *al,void *value);
        void    *(*get_elem)    (ARRAYLIST *al,int index);
        void     (*remove_elem) (ARRAYLIST *al,int index);
	void	*(*get_first)	(ARRAYLIST *al);
	void	*(*get_last)	(ARRAYLIST *al);
	void	*(*get_next)	(ARRAYLIST *al);
	void	*(*get_prev)	(ARRAYLIST *al);
	int	 (*is_empty)	(ARRAYLIST *al);
	int	 (*size)	(ARRAYLIST *al);
	void	 (*set_iterator)(ARRAYLIST *al);
};
