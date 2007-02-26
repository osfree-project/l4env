/*** BOX STYLES ***/
#define GFX_BOX_DESKBG	0	/* for desktop background */
#define GFX_BOX_WINBG  	1	/* for window backgrounds */
#define GFX_BOX_ACT 	2	/* for active objects (activator button) */
#define GFX_BOX_SEL 	3	/* for selective objects (selection button) */
#define GFX_BOX_FOCUS	4	/* for object under mouse */

/*** FRAME STYLES ***/
#define GFX_FRAME_RAISED 	0
#define GFX_FRAME_SUNKEN 	1
#define GFX_FRAME_RIDGE  	2
#define GFX_FRAME_GROOVE 	3
#define GFX_FRAME_PRESSED 	4
#define GFX_FRAME_FOCUS		5

/*** STRING STYLES ***/
#define GFX_STRING_DEFAULT	0
#define GFX_STRING_BLACK	1

struct basegfx_services {
	void	(*draw_box)				(s16 x1,s16 y1,s16 x2,s16 y2,u16 style);
	void	(*draw_frame) 			(s16 x1,s16 y1,s16 x2,s16 y2,u16 style);
	void	(*draw_string)			(s16 x, s16 y, u32 font_id,u16 style,u8 *str);
	void	(*draw_ansi)			(s16 x, s16 y, u32 font_id,u8 *str,u8 *bgfg);
	void	(*update_properties)	(void);
};
