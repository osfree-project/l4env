#if !defined(HASHTAB)
#define HASHTAB void
#endif

struct hashtab_services {
	HASHTAB	*(*create)		(u32 tab_size,u32 max_hash_length);
	void	 (*destroy)		(HASHTAB *h);
	void	 (*add_elem)	(HASHTAB *h,char *ident,void *value);
	void	*(*get_elem)	(HASHTAB *h,char *ident);
	void	 (*remove_elem)	(HASHTAB *h,char *ident);
};
