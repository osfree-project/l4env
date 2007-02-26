struct image16 {
	s32 w,h;			/* width and height of the image */
	u16	*pixels;		/* 16bit color values of the pixels */
	s32	cache_index;	/* internal use of data handlers */
	s32	cache_ident;	/* internal use of data handlers */
};


struct image16_services {
	void			 (*update_properties) (void);
	struct image16	*(*create)			(s32 width,s32 height);
	void			 (*destroy)			(struct image16 *img);
	void			 (*paint)			(s32 x,s32 y,struct image16 *img);
	void			 (*paint_scaled)	(s32 x,s32 y,s32 w,s32 h,struct image16 *img);
	s32			 	 (*size_of)			(struct image16 *img);
	void			 (*clear)			(struct image16 *img);
};
