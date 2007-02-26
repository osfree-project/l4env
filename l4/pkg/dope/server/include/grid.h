#if !defined(GRID)
#define GRID struct public_grid
#endif

struct grid_methods;

struct public_grid {
	struct widget_methods	*gen;
	struct grid_methods 	*grid;
};

struct grid_methods {
	void (*add)				(GRID *,WIDGETARG *new_child);
	void (*remove)			(GRID *,WIDGETARG *child);
	
	void (*set_row)			(GRID *,WIDGETARG *child,s32 row_idx);
	s32	 (*get_row)			(GRID *,WIDGETARG *child);
	void (*set_col)			(GRID *,WIDGETARG *child,s32 col_idx);
	s32	 (*get_col)			(GRID *,WIDGETARG *child);	

	void (*set_row_span)	(GRID *,WIDGETARG *child,s32 num_rows);
	s32	 (*get_row_span)	(GRID *,WIDGETARG *child);
	void (*set_col_span)	(GRID *,WIDGETARG *child,s32 num_cols);
	s32	 (*get_col_span)	(GRID *,WIDGETARG *child);	

	void (*set_pad_x)		(GRID *,WIDGETARG *child,s32 pad_x);
	s32	 (*get_pad_x)		(GRID *,WIDGETARG *child);
	void (*set_pad_y)		(GRID *,WIDGETARG *child,s32 pad_y);
	s32	 (*get_pad_y)		(GRID *,WIDGETARG *child);	
	
	void (*set_row_h)		(GRID *,u32 row,u32 row_height);
	u32	 (*get_row_h)		(GRID *,u32 row);
	void (*set_col_w)		(GRID *,u32 col,u32 col_width);
	u32	 (*get_col_w)		(GRID *,u32 col);

	void (*set_row_weight)	(GRID *,u32 row,float row_weight);
	float(*get_row_weight)	(GRID *,u32 row);
	void (*set_col_weight)	(GRID *,u32 col,float col_weight);
	float(*get_col_weight)	(GRID *,u32 col);
	
};

struct grid_services {
	GRID	*(*create)	(void);
};
