struct image8i {
	s32 w,h;			/* width and height of the image */
	u32	*palette;		/* colormap with 256 entries */
	u8	*pixels;		/* color indices of the pixels */
	s32	cache_index;	/* only used by the 'Image8iData' module */
	s32	cache_ident;	/* only used by the "Image8iData' module */
};


struct image8i_services {
	void			 (*set_cache)			(s32 max_entries,s32 max_size);
	void			 (*update_properties)	(void);
	void			 (*paint)				(s32 x,s32 y,struct image8i *img);
	void			 (*paint_scaled)		(s32 x,s32 y,s32 w,s32 h,struct image8i *img);
	void			 (*refresh)				(struct image8i *img);
	struct image8i	*(*create)				(s32 width,s32 height);
	void			 (*destroy)				(struct image8i *img);
	s32			 	 (*size_of)				(struct image8i *img);
};
