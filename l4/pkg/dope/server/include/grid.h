/*
 * \brief   Interface of DOpE Grid widget module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

struct grid_methods;
struct grid_data;

#define GRID struct grid

#define GRID_STICKY_EAST  0x01
#define GRID_STICKY_WEST  0x02
#define GRID_STICKY_NORTH 0x04
#define GRID_STICKY_SOUTH 0x08

struct grid {
	struct widget_methods *gen;
	struct grid_methods   *grid;
	struct widget_data    *wd;
	struct grid_data      *gd;
};

struct grid_methods {
	void (*add)     (GRID *,WIDGETARG *new_child);
	void (*remove)  (GRID *,WIDGETARG *child);
	
	void (*set_row) (GRID *,WIDGETARG *child,s32 row_idx);
	s32  (*get_row) (GRID *,WIDGETARG *child);
	void (*set_col) (GRID *,WIDGETARG *child,s32 col_idx);
	s32  (*get_col) (GRID *,WIDGETARG *child);

	void (*set_row_span) (GRID *,WIDGETARG *child,s32 num_rows);
	s32  (*get_row_span) (GRID *,WIDGETARG *child);
	void (*set_col_span) (GRID *,WIDGETARG *child,s32 num_cols);
	s32  (*get_col_span) (GRID *,WIDGETARG *child);

	void (*set_pad_x)  (GRID *,WIDGETARG *child,s32 pad_x);
	s32  (*get_pad_x)  (GRID *,WIDGETARG *child);
	void (*set_pad_y)  (GRID *,WIDGETARG *child,s32 pad_y);
	s32  (*get_pad_y)  (GRID *,WIDGETARG *child);
	
	void (*set_sticky) (GRID *,WIDGETARG *child,s32 sticky);
	s32  (*get_sticky) (GRID *,WIDGETARG *child);
	
	void (*set_row_h)  (GRID *,u32 row,u32 row_height);
	u32  (*get_row_h)  (GRID *,u32 row);
	void (*set_col_w)  (GRID *,u32 col,u32 col_width);
	u32  (*get_col_w)  (GRID *,u32 col);

	void (*set_row_weight) (GRID *,u32 row,float row_weight);
	float(*get_row_weight) (GRID *,u32 row);
	void (*set_col_weight) (GRID *,u32 col,float col_weight);
	float(*get_col_weight) (GRID *,u32 col);
};

struct grid_services {
	GRID *(*create) (void);
};
