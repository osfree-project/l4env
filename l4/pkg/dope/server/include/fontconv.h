struct fontconv_services {
	s16	 (*probe)				(void *fontadr);
	void	 (*gen_width_table)		(void *fontadr,s32 *dst_wtab);
	void	 (*gen_offset_table)	(void *fontadr,s32 *dst_otab);
	u8	*(*get_name)			(void *fontadr);
	u16	 (*get_top)				(void *fontadr);
	u16	 (*get_bottom)			(void *fontadr);
	u32	 (*get_image_width)		(void *fontadr);
	u32	 (*get_image_height)	(void *fontadr);
	void	 (*gen_image)			(void *fontadr,u8 *dst_img);
};
