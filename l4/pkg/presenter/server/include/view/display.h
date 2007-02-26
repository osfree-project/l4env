struct pres_display_services  {
int	(*show_slide)		(char *addr, int addr_buf_size, u16 *dst,s32 img_w,s32 img_h);
int	(*check_slide)		(char *addr, int addr_buf_size);
};

