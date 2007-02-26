struct font_struct {
	s32			font_id;
	s32			*width_table;
	s32			*offset_table;
	s32			img_w,img_h;
	s16			top,bottom;
	u8			*image;
	u8			*name;
};


struct fontman_services {
	struct font_struct *(*get_by_id)		(s32 font_id);
	s32				(*calc_str_width)	(s32 font_id,char *str);
	s32				(*calc_str_height)	(s32 font_id,char *str);
};
