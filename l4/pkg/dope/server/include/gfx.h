#define GFX_IMG_TYPE_RGB565    1
#define GFX_IMG_TYPE_RGBA8888  2

struct gfx_services {
	s32 (*alloc_scr) (char *scrmode);
	s32 (*alloc_img) (s16 w, s16 h, s32 img_type);
	s32 (*alloc_pal) (s16 num_entries);
	s32 (*alloc_fnt) (char *fntname);
	
	s32 (*get_width)  (s32 ds_id);
	s32 (*get_height) (s32 ds_id);
	s32 (*get_depth)  (s32 ds_id);
	
	void  (*free)   (s32 ds_id);
	void *(*map)    (s32 ds_id);
	void  (*unmap)  (s32 ds_id);
	void  (*update) (s32 ds_id, s32 x, s32 y, s32 w, s32 h);
	
	void (*draw_hline)  (s32 ds_id, s16 x, s16 y, s16 w, u32 rgba);
	void (*draw_vline)  (s32 ds_id, s16 x, s16 y, s16 h, u32 rgba);
	void (*draw_fill)   (s32 ds_id, s16 x, s16 y, s16 w, s16 h, u32 rgba);
	void (*draw_grad)   (s32 ds_id, s16 x, s16 y, s16 w, s16 h, float dr, float dg, float db, float da, u32 rgba);
	void (*draw_idximg) (s32 ds_id, s16 x, s16 y, s16 w, s16 h, s32 idximg_id, s32 pal_id, u8 alpha);
	void (*draw_img)    (s32 ds_id, s16 x, s16 y, s16 w, s16 h, s32 img_id, u8 alpha);
	void (*draw_string) (s32 ds_id, s16 x, s16 y, s32 fg_rgba, u32 bg_rgba, s32 fnt_id, char *str);
	void (*draw_ansi)   (s32 ds_id, s16 x, s16 y, s32 fnt_id, char *str, u8 *bgfg);
	
	void  (*set_clipping) (s32 ds_id, s32 x, s32 y, s32 w, s32 h);
	
	void  (*set_mouse_cursor) (s32 scr_id, s32 img32_id);
	void  (*set_mouse_pos)    (s32 scr_id, s32 x, s32 y);

	/* benchmarking */
}
