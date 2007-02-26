#if !defined(HASHTAB)
#define HASHTAB void
#endif

struct hashtab_services {
	HASHTAB	*(*create)		(u32 tab_size);
	void	 (*destroy)		(HASHTAB *h);
	void	 (*add_elem)	(HASHTAB *h,int ident,void *value);
	void	*(*get_elem)	(HASHTAB *h,int ident);
	void	 (*remove_elem)	(HASHTAB *h,int ident);
};
