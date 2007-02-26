#if !defined(FRAME)
#define FRAME struct public_frame
#endif

struct frame_methods;
struct public_frame {
	struct widget_methods 	*gen;
	struct frame_methods 	*frame;
};

struct frame_methods {
	void	 (*set_content)		(FRAME *,WIDGETARG *new_content);
	WIDGET	*(*get_content)		(FRAME *);
	void	 (*set_scrollx)		(FRAME *,u32 flag);
	s32	 	 (*get_scrollx)		(FRAME *);
	void	 (*set_scrolly)		(FRAME *,u32 flag);
	s32	 	 (*get_scrolly)		(FRAME *);
	void	 (*set_fitx)		(FRAME *,u32 flag);
	s32		 (*get_fitx)		(FRAME *);
	void	 (*set_fity)		(FRAME *,u32 flag);
	s32		 (*get_fity)		(FRAME *);
	void	 (*set_xview)		(FRAME *,s32 xview);
	s32		 (*get_xview)		(FRAME *);
	void	 (*set_yview)		(FRAME *,s32 yview);
	s32		 (*get_yview)		(FRAME *);
	void	 (*set_background)	(FRAME *,u32 bg);
	u32		 (*get_background)	(FRAME *);
	void	 (*xscroll_update)	(FRAME *,u16 redraw_flag);
	void	 (*yscroll_update)	(FRAME *,u16 redraw_flag);
};


struct frame_services {
	FRAME	*(*create)	(void);
};
