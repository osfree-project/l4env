#if !defined(CACHE)
#define CACHE void
#endif

struct cache_services {
	CACHE	*(*create)		(s32 max_entries,s32 max_size);
	void	 (*destroy)		(CACHE *c);
	s32	 	 (*add_elem)	(CACHE *c,void *elem,s32 elemsize,s32 ident,void (*destroy)(void *));
	void	*(*get_elem)	(CACHE *c,s32 index,s32 ident);
	void	 (*remove_elem)	(CACHE *c,s32 index);
};
