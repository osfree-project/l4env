struct memory_services {
	void	*(*alloc)	(long size);
	void	 (*free)	(void *memadr);
	void	*(*move)	(void *dst,void *src,s32 size);
};
