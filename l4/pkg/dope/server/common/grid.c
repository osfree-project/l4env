/*
 * \brief	DOpE Grid widget module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 *
 * The Grid  layout  widget  enables the  placement
 * of  multiple  child-widgets on a  grid.  Row and 
 * column  sizes  can  be  specified  absolutely or 
 * weighted.   Each  widget  can  be   individually 
 * positioned using padding, spanning and alignment
 */


struct private_grid;
#define GRID struct private_grid
#define WIDGET GRID
#define WIDGETARG WIDGET

#include "dope-config.h"
#include "memory.h"
#include "widget_data.h"
#include "widget.h"
#include "background.h"
#include "clipping.h"
#include "widman.h"
#include "script.h"
#include "grid.h"

static struct memory_services 		*mem;
static struct widman_services 		*widman;
static struct clipping_services		*clip;
static struct background_services	*bg;
static struct script_services		*script;

#define GRID_UPDATE_COL_LAYOUT 	0x01
#define GRID_UPDATE_ROW_LAYOUT 	0x02
#define GRID_UPDATE_NEW_CHILD	0x04

#define GRID_SECTION_WEIGHTED 	0x00
#define GRID_SECTION_FIXED		0x01

#define GRID_STICKY_EAST		0x01
#define GRID_STICKY_WEST		0x02
#define GRID_STICKY_NORTH		0x04
#define GRID_STICKY_SOUTH		0x08

struct section_struct;
struct section_struct {
	s32					type;	/* fixed size or weight */
	s32					size;	/* row size in pixels */
	s32					offset;	/* position relative to grid parent */
	s32					index;	/* index of row/column */
	float					weight;	/* weight of row/column */
	struct section_struct  *next;	/* next row/column in connected list*/
};


struct cell_struct;
struct cell_struct {
	struct section_struct	*row;		/* first row used by the cell */
	struct section_struct	*col;		/* first column used be the cell */
	s16					 row_span;	/* number of rows used by the cell */
	s16					 col_span;	/* number of colums used by the cell */
	u16					 sticky;	/* alignment of widget inside its cell */
	s16					 pad_x;		/* hor.distance of widget to cell border */
	s16					 pad_y;		/* ver.distance of widget to cell border */
	WIDGET					*wid;		/* associated widget */
	struct cell_struct		*next;		/* next cell in connected cell-list */
};


GRID {
	/* entry must point to a general widget interface */
	struct widget_methods 	*gen;	/* for public access */
	
	/* entry is for the ones who knows the real widget identity (grid) */
	struct grid_methods 	*grid;	/* for dedicated users */
	
	/* entry contains general widget data */
	struct widget_data		*wd; 	/* access for grid module and widget manager */
	
	/* here comes the private grid specific data */
	struct section_struct	*rows;	/* list of rows */
	struct section_struct	*cols;	/* list of columns */
	struct cell_struct		*cells;	/* list of cells */
	u32					 update;/* grid specific update flags */
};


int init_grid(struct dope_services *d);
void print_grid_info(GRID *g);

/**********************************/
/*** FUNCTIONS FOR INTERNAL USE ***/
/**********************************/


void print_grid_info(GRID *g) {

	struct section_struct *sec;
	struct cell_struct *cc;
	WIDGET *cw;

	printf("Grid info:\n");
	if (!g) {
		printf(" grid is zero!\n");
	}
	printf(" xpos   = %lu\n",(long)g->wd->x);
	printf(" ypos   = %lu\n",(long)g->wd->y);
	printf(" width  = %lu\n",(long)g->wd->w);
	printf(" height = %lu\n",(long)g->wd->h);
	printf(" row-sections:\n");
	sec=g->rows;
	while (sec) {
		printf("  index: %lu\n",sec->index);
		printf("   offset:%lu\n",sec->offset);
		printf("   size:  %lu\n",sec->size);
		printf("   type:  %lu\n",sec->type);
		printf("   weight:%f\n", sec->weight);
		sec=sec->next;
	}
	printf(" column-sections:\n");
	sec=g->cols;
	while (sec) {
		printf("  index: %lu\n",sec->index);
		printf("   offset:%lu\n",sec->offset);
		printf("   size:  %lu\n",sec->size);
		printf("   type:  %lu\n",sec->type);
		printf("   weight:%f\n", sec->weight);
		sec=sec->next;
	}
	printf(" child-widgets:\n");
	cc=g->cells;
	while (cc) {
		cw=cc->wid;
		if (cw) {
			printf("  position (");
			if (cc->row) printf("%lu",cc->row->index);
			else printf("unknown");
			printf(",");
			if (cc->col) printf("%lu",cc->col->index);
			else printf("unknown");
			printf(")\n");
			printf("   xpos   = %lu\n",cw->gen->get_x(cw));
			printf("   ypos   = %lu\n",cw->gen->get_y(cw));
			printf("   width  = %lu\n",cw->gen->get_w(cw));
			printf("   height = %lu\n",cw->gen->get_h(cw));
		}
		cc=cc->next;
		printf("aaa\n");
	}
	printf("ok.\n");
}


/*** CREATE NEW ROW/COLUMN SECTION WITH DEFAULT VALUES ***/
static struct section_struct *new_section(u32 index) {
	struct section_struct *new = mem->alloc(sizeof(struct section_struct));

	DOPEDEBUG(printf("Grid(new_section): index = %lu\n",index);)
	if (!new) {
		DOPEDEBUG(printf("Grid(new_section): out of memory!\n");)
		return NULL;
	}
	/* set default properties */
	new->type=GRID_SECTION_WEIGHTED;
	new->size=64.0;
	new->offset=0;
	new->index=index;
	new->weight=1.0;
	new->next=NULL;
	return new;
}


/*** RETURNS ROW/COLUMN SECTION OF A SECTION-LIST BY ITS INDEX ***/
/* the function creates a new section if the index doesnt exist already */
static struct section_struct *get_section(struct section_struct *curr,s32 idx) {
	struct section_struct *new;
	
	if (curr->index == idx) return curr;
		
	/* search for the needed index */
	while (curr->next) {
		if (curr->next->index == idx) return curr->next;
		if (curr->next->index > idx) break;
		curr=curr->next;			
	}
	
	/* insert new element */
	new=new_section(idx);
	if (new) {
		new->next=curr->next;
		curr->next=new;
	}
	return new;
}


/*** RETURNS ROW SECTION STRUCTURE BY INDEX ***/
/* a new row section is created automaticaly if needed */
static struct section_struct *get_row(GRID *g,s32 row_idx) {
	struct section_struct *new;
	
	if (!g) return NULL;
	if (!g->rows) {
		g->rows=new_section(row_idx);
		return g->rows;
	}
	if (g->rows->index > row_idx) {
		new=new_section(row_idx);
		if (new) {
			new->next=g->rows;
			g->rows=new;
		}
		return new;
	}
	return get_section(g->rows,row_idx);
}


/*** RETURNS COLUMN SECTION STRUCTURE BY INDEX ***/
/* a new column section is created automaticaly if needed */
static struct section_struct *get_column(GRID *g,s32 col_idx) {
	struct section_struct *new;
	
	if (!g) return NULL;
	if (!g->cols) {
		g->cols=new_section(col_idx);
		return g->cols;
	}
	if (g->cols->index > col_idx) {
		new=new_section(col_idx);
		if (new) {
			new->next=g->cols;
			g->cols=new;
		}
		return new;
	}
	return get_section(g->cols,col_idx);
}


/*** RETURNS THE GRID-CELL THAT IS ASSOCIATED WITH A GIVEN WIDGET ***/
static struct cell_struct *get_cell(GRID *g,WIDGET *w) {
	struct cell_struct *curr;
	if (!g || !w) return NULL;
	if (w->gen->get_parent(w)!=g) return NULL;
	curr=g->cells;
	while (curr) {
		if (curr->wid == w) return curr;
		curr=curr->next;
	}
	return NULL;
}


/*** CALCULATE THE SIZES OF ROWS/COLUMNS-SECTIONS ***/
/* seclist: 	begin of section list				*/
/* sum_size:	desired overall size				*/
/* num_secs:	max.number of sections to modify	*/
static void  calc_section_sizes(struct section_struct *seclist,
								s32 sum_size,u32 num_secs) {
	struct section_struct *curr;
	float	sum_weights = 0.0;
	s32	i;
	s32	fixed_size = 0;
	s32	rest = (float)sum_size;
	s32	weight_size;
	if (!seclist) return;
	
	/* calculate sum of section weights and size of fixed-size-sections */
	curr=seclist;
	for (i=num_secs;(i--) && (curr);) {
		if (curr->type == GRID_SECTION_WEIGHTED) {
			sum_weights = sum_weights + curr->weight;
		}
		if (curr->type == GRID_SECTION_FIXED) {
			fixed_size = fixed_size + curr->size;
		}
		curr=curr->next;
	}
	if (sum_weights == 0.0) sum_weights = 0.0001;
	
	/* calculate space that is left for the weighted sections */
	weight_size = sum_size - fixed_size;
	if (weight_size<0.0) weight_size = 0.0;
	
	/* calculate sizes of weighted sections */
	curr=seclist;
	for (i=num_secs;(i--) && (curr);) {
		if (curr->type == GRID_SECTION_WEIGHTED) {
			curr->size = (s32)((curr->weight * (float)weight_size)/sum_weights);
			weight_size = weight_size - curr->size;
			sum_weights = sum_weights - curr->weight;
			if (curr->size<0) curr->size=0;
			if (sum_weights<=0.0) sum_weights=0.0001;
			if (weight_size<=0.0) weight_size=0.0;
		}
		if (curr->next) rest -= curr->size;
		else curr->size = rest;
		curr=curr->next;
	}
}


/*** CALCULATE OFFSETS OF SECTIONS RELATIVE TO THE FIRST SECTION ***/
static void calc_section_offsets(struct section_struct *curr) {
	s32	curr_offset=0;
	if (!curr) return;
	while (curr) {
		curr->offset = curr_offset;
		curr_offset = curr_offset + curr->size;
		curr = curr->next;
	}
}


/*** RETURN SIZE OF THE SPECIFIED NUMBER OF NEIGHBOUR SECTIONS ***/
static s32 get_section_size(struct section_struct *curr,u32 num_sections) {
	if (!curr) return 0;
	if (!num_sections) return 0;
//	printf("get_section_size: curr->index = %lu\n",curr->index);
	if (num_sections>1) return curr->size + get_section_size(get_section(curr,curr->index+1),num_sections-1);
	else return curr->size;
}


/*** INCREMENT SECTION SIZES SO THAT THE WIDGETS FIT INTO ITS CELLS ***/
static void fit_widgets(GRID *g) {
	struct cell_struct *cc;	/* current cell */
	float  min;
	float  secsize;
	u16 update_widget;
	if (!g) return;
	cc=g->cells;
	while (cc) {
		if (cc->wid && cc->row && cc->col) {
			update_widget=0;

			/* check if columns are big enough to contain the cell */
			min = cc->wid->wd->min_w + 2*cc->pad_x;
			secsize = get_section_size(cc->col,cc->col_span);
			if (secsize < min) {
				calc_section_sizes(cc->col,min,cc->col_span);
				secsize = min;
			}
			
			/* shrink widgets width so that it fits horizontally into its cell */
			if (cc->wid->wd->w > secsize - 2*cc->pad_x) {
				cc->wid->gen->set_w(cc->wid,secsize - 2*cc->pad_x);
				update_widget=1;
			}

			/* check if rows are big enough to contain the cell */
			min = cc->wid->wd->min_h + 2*cc->pad_y;
			secsize = get_section_size(cc->row,cc->row_span);
			if (secsize < min) {
				calc_section_sizes(cc->row,min,cc->row_span);
				secsize = min;
			}

			/* shrink widgets height so that it fits vertically into its cell */
			if (cc->wid->wd->h > secsize - 2*cc->pad_y) {
				cc->wid->gen->set_h(cc->wid,secsize - 2*cc->pad_y);
				update_widget=1;
			}
			
			/* did we changed the widgets properties? */
			if (update_widget) cc->wid->gen->update(cc->wid,WID_UPDATE_HIDDEN);
		}	
		cc=cc->next;
	}
}


/*** SET POSITIONS OF GRID WIDGETS TO ITS CELLS POSITIONS ***/
static void place_widgets(GRID *g) {
	struct cell_struct *cc;	/* current cell */
	WIDGET *cw;				/* current widget */
	s32 cell_x,cell_y,cell_w,cell_h;
	s32 wid_x,wid_y,wid_w,wid_h;
	if (!g) return;
	cc=g->cells;
	while (cc) {
		if (cc->col && cc->row) {
			cell_x = cc->col->offset + cc->pad_x;
			cell_y = cc->row->offset + cc->pad_y;
			cell_w = get_section_size(cc->col,cc->col_span) - (float)(2*cc->pad_x);
			cell_h = get_section_size(cc->row,cc->row_span) - (float)(2*cc->pad_y);
			cw=cc->wid;
			
			wid_w = cw->gen->get_w(cw);
			wid_x = cell_x + ((cell_w - wid_w)>>1);
			cw->gen->set_x(cw,wid_x);
	
			wid_h = cw->gen->get_h(cw);
			wid_y = cell_y + ((cell_h - wid_h)>>1);
			cw->gen->set_y(cw,wid_y);
			
			if (cc->sticky & GRID_STICKY_EAST) {
				wid_w = cell_w - wid_x - wid_w;
				cw->gen->set_w(cw,wid_w);
			}

			if (cc->sticky & GRID_STICKY_WEST) {
				wid_x = cell_x;wid_w = cell_w;
				cw->gen->set_x(cw,wid_x);
				cw->gen->set_w(cw,wid_w);
			}

			if (cc->sticky & GRID_STICKY_SOUTH) {
				wid_h = cell_h - wid_y - wid_h;
				cw->gen->set_h(cw,wid_h);
			}

			if (cc->sticky & GRID_STICKY_NORTH) {
				wid_y = cell_y;wid_h = cell_h;
				cw->gen->set_y(cw,wid_y);
				cw->gen->set_h(cw,wid_h);
			}
			cw->gen->update(cw,WID_UPDATE_HIDDEN);
		}
		cc=cc->next;
	}
}


/******************************/
/*** GENERAL WIDGET METHODS ***/
/******************************/

static void grid_draw(GRID *g,long x,long y) {

	long x1,y1,x2,y2;
	WIDGET *cw;
	struct cell_struct *cc;
	
	if (!g) {
		DOPEDEBUG(printf("Grid(grid_draw): grid is zero!\n"));
		return;
	}
	cc=g->cells;
	x += g->wd->x;
	y += g->wd->y;
	while (cc) {
		cw=cc->wid;
		if (cw && cc->col && cc->row) {
			x1 = cc->col->offset + x;
			y1 = cc->row->offset + y;
			x2 = x1 + get_section_size(cc->col,cc->col_span) - 1;
			y2 = y1 + get_section_size(cc->row,cc->row_span) - 1;
			clip->push(x1,y1,x2,y2);
			cw->gen->draw(cw,x,y);
			clip->pop();
		}
		cc=cc->next;
	}
}


static WIDGET *grid_find(GRID *g,long x,long y) {
	struct cell_struct *cc;
	WIDGET *cw;
	WIDGET *result;
	if (!g) return NULL;

	x -= g->wd->x;
	y -= g->wd->y;
	
	/* check if position is inside the window */
	if ((x >= 0) && (y >= 0) && (x < g->wd->w) && (y < g->wd->h)) {
	
		/* go through all cells and check their widgets */
		cc=g->cells;
		while (cc) {
			cw=cc->wid;
			if (cw) {
				result=cw->gen->find(cw, x, y);
				if (result) return result;
			}
			cc=cc->next;
		}
		return g;
	}
	return NULL;	
}


static void (*orig_update)(GRID *g,u16 redraw_flag);

static void grid_update(GRID *g,u16 redraw_flag) {
	if (!g) return;

	if ((g->wd->update & WID_UPDATE_SIZE) | (g->update & GRID_UPDATE_NEW_CHILD)) {
		g->update = g->update | GRID_UPDATE_COL_LAYOUT | GRID_UPDATE_ROW_LAYOUT;	
	}

	/* calculate sizes of rows/columns */
	if (g->update & GRID_UPDATE_COL_LAYOUT) {
		calc_section_sizes(g->cols,g->wd->w,99999);
	}
	if (g->update & GRID_UPDATE_ROW_LAYOUT) {
		calc_section_sizes(g->rows,g->wd->h,99999);
	}

	/* expand rows and columns so that widgets fit into their cells */
	fit_widgets(g);

	/* calculate offsets of rows/columns */
	if (g->update & GRID_UPDATE_COL_LAYOUT) {
		calc_section_offsets(g->cols);
	}
	if (g->update & GRID_UPDATE_ROW_LAYOUT) {
		calc_section_offsets(g->rows);
	}
	
	/* set widget positions */
	place_widgets(g);
	
	/* draw grid */
	if (redraw_flag && !(g->wd->update & (WID_UPDATE_POS | WID_UPDATE_SIZE))) {
		g->gen->force_redraw(g);
	}
	orig_update(g,redraw_flag);
}



/*****************************/
/*** GRID SPECIFIC METHODS ***/
/*****************************/



/*** ADD NEW CHILD WIDGET TO GRID ***/
static void grid_add(GRID *g,WIDGET *new_elem) {
	struct cell_struct *new_cell;

//	printf("I am here!\n");

	if (!g) return;
	if (!new_elem) return;
	if (new_elem->gen->get_parent(new_elem)) return;

//	printf("I am here2!\n");

	
	/* create new cell with default values */
	/* the widget has no row/column information yet */
	/* it will appear as soon as the row/column values are defined */
	new_cell = (struct cell_struct *)mem->alloc(sizeof(struct cell_struct));
	if (!new_cell) {
		DOPEDEBUG(printf("Grid(grid_add): out of memory during creation of a grid cell\n");)
		return;
	}

//	printf("I am here3!\n");

	new_cell->row = NULL;
	new_cell->col = NULL;
	new_cell->row_span = 1;
	new_cell->col_span = 1;
	new_cell->sticky =  GRID_STICKY_NORTH | GRID_STICKY_SOUTH | 
						GRID_STICKY_EAST | GRID_STICKY_WEST;
	new_cell->pad_x = 0;
	new_cell->pad_y = 0;
	new_cell->wid = new_elem;
	new_elem->gen->inc_ref(new_elem);
	new_elem->gen->set_parent(new_elem,g);
	new_cell->next = g->cells;
	g->cells = new_cell;

//	printf("I am here4!\n");
	
	g->update = g->update | GRID_UPDATE_NEW_CHILD;	
	grid_update(g,WID_UPDATE_REDRAW);

//	printf("I am here5!\n");
}


/*** REMOVE CHILD WIDGET FROM GRID ***/
static void grid_remove(GRID *g,WIDGET *element) {
	struct cell_struct *prev_cell;
	struct cell_struct *cell = get_cell(g,element);
	if (!cell) return;
	if (!element) return;
	element->gen->dec_ref(element);
	cell->wid=NULL;

	g->update = g->update | GRID_UPDATE_NEW_CHILD;	
	
	/* is cell first element of cell list? */
	if (cell == g->cells) {
		g->cells = cell->next;
		mem->free(cell);
		return;
	}
	
	/* find previous cell in cell-list */
	prev_cell=g->cells;
	while (prev_cell->next) {
		if (prev_cell->next == cell) {
			prev_cell->next = cell->next;
			mem->free(cell);
			return;
		}
	}
}


/*** GET/SET ROW OF A CHILD ***/
static void grid_set_row(GRID *g,WIDGET *w,s32 row_idx) {
	struct cell_struct *cell = get_cell(g,w);
	if (!cell) return;
	cell->row = get_row(g,row_idx);
	if (cell->row && cell->col) g->update = g->update | GRID_UPDATE_NEW_CHILD;
}
static s32 grid_get_row(GRID *g,WIDGET *w) {
	struct cell_struct *cell = get_cell(g,w);
	if (!cell) return -1;
	if (!cell->row) return -1;
	return cell->row->index;
}


/*** GET/SET COLUMN OF A CHILD ***/
static void grid_set_col(GRID *g,WIDGET *w,s32 col_idx) {
	struct cell_struct *cell = get_cell(g,w);
	if (!cell) return;
	cell->col = get_column(g,col_idx);
	if (cell->row && cell->col) g->update = g->update | GRID_UPDATE_NEW_CHILD;
}
static s32 grid_get_col(GRID *g,WIDGET *w) {
	struct cell_struct *cell = get_cell(g,w);
	if (!cell) return -1;
	if (!cell->col) return -1;
	return cell->col->index;
}


/*** GET/SET ROWSPAN OF A CHILD ***/
static void grid_set_row_span(GRID *g,WIDGET *w,s32 num_rows) {
	struct cell_struct *cell = get_cell(g,w);
	if (!cell) return;
	cell->row_span = num_rows;
	g->update = g->update | GRID_UPDATE_ROW_LAYOUT;
}
static s32 grid_get_row_span(GRID *g,WIDGET *w) {
	struct cell_struct *cell = get_cell(g,w);
	if (!cell) return -1;
	if (!cell->row) return -1;
	return cell->row_span;
}


/*** GET/SET COLUMNSPAN OF A CHILD ***/
static void grid_set_col_span(GRID *g,WIDGET *w,s32 num_cols) {
	struct cell_struct *cell = get_cell(g,w);
	if (!cell) return;
	cell->col_span = num_cols;
	g->update = g->update | GRID_UPDATE_COL_LAYOUT;
}
static s32 grid_get_col_span(GRID *g,WIDGET *w) {
	struct cell_struct *cell = get_cell(g,w);
	if (!cell) return -1;
	if (!cell->col) return -1;
	return cell->col_span;
}


/*** GET/SET PAD-X OF A CHILD ***/
static void grid_set_pad_x(GRID *g,WIDGET *w,s32 pad_x) {
	struct cell_struct *cell = get_cell(g,w);
	if (!cell) return;
	cell->pad_x = pad_x;
	g->update = g->update | GRID_UPDATE_ROW_LAYOUT;
}
static s32 grid_get_pad_x(GRID *g,WIDGET *w) {
	struct cell_struct *cell = get_cell(g,w);
	if (!cell) return -1;
	if (!cell->row) return -1;
	return cell->pad_x;
}


/*** GET/SET PAD-Y OF A CHILD ***/
static void grid_set_pad_y(GRID *g,WIDGET *w,s32 pad_y) {
	struct cell_struct *cell = get_cell(g,w);
	if (!cell) return;
	cell->pad_y = pad_y;
	g->update = g->update | GRID_UPDATE_COL_LAYOUT;
}
static s32 grid_get_pad_y(GRID *g,WIDGET *w) {
	struct cell_struct *cell = get_cell(g,w);
	if (!cell) return -1;
	if (!cell->col) return -1;
	return cell->pad_y;
}


/*** GET/SET ROW PIXEL SIZE ***/
static void grid_set_row_h(GRID *g,u32 row_idx,u32 row_height) {
	struct section_struct *row = get_row(g,row_idx);
	if (!row) return;
	row->size = row_height;
	row->type = GRID_SECTION_FIXED;
	g->update = g->update | GRID_UPDATE_ROW_LAYOUT;
}
static u32 grid_get_row_h(GRID *g,u32 row_idx) {
	struct section_struct *row = get_row(g,row_idx);
	if (!row) return 0;
	return row->size;
}


/*** GET/SET COLUMN PIXEL SIZE ***/
static void grid_set_col_w(GRID *g,u32 col_idx,u32 col_width) {
	struct section_struct *col = get_column(g,col_idx);
	if (!col) return;
	col->size = col_width;
	col->type = GRID_SECTION_FIXED;
	g->update = g->update | GRID_UPDATE_COL_LAYOUT;
}
static u32 grid_get_col_w(GRID *g,u32 col_idx) {
	struct section_struct *col = get_column(g,col_idx);
	if (!col) return 0;
	return col->size;
}


/*** GET/SET ROW WEIGHT ***/
static void grid_set_row_weight(GRID *g,u32 row_idx,float row_weight) {
	struct section_struct *row = get_row(g,row_idx);
	if (!row) return;
	row->weight=row_weight;
	row->type=GRID_SECTION_WEIGHTED;
	g->update = g->update | GRID_UPDATE_ROW_LAYOUT;
}
static float grid_get_row_weight(GRID *g,u32 row_idx) {
	struct section_struct *row = get_row(g,row_idx);
	if (!row) return 0;
	return row->weight;
}


/*** GET/SET COLUMN WEIGHT ***/
static void grid_set_col_weight(GRID *g,u32 col_idx,float col_weight) {
	struct section_struct *col = get_column(g,col_idx);
	if (!col) return;
	col->weight = col_weight;
	col->type =GRID_SECTION_WEIGHTED;
	g->update = g->update | GRID_UPDATE_COL_LAYOUT;
}
static float grid_get_col_weight(GRID *g,u32 col_idx) {
	struct section_struct *col = get_column(g,col_idx);
	if (!col) return 0;
	return col->weight;
}


static struct widget_methods 	gen_methods;
static struct grid_methods 		grd_methods={
	grid_add,
	grid_remove,
	grid_set_row,
	grid_get_row,
	grid_set_col,
	grid_get_col,
	grid_set_row_span,
	grid_get_row_span,
	grid_set_col_span,
	grid_get_col_span,
	grid_set_pad_x,
	grid_get_pad_x,
	grid_set_pad_y,
	grid_get_pad_y,
	grid_set_row_h,
	grid_get_row_h,
	grid_set_col_w,
	grid_get_col_w,
	grid_set_row_weight,
	grid_get_row_weight,
	grid_set_col_weight,
	grid_get_col_weight,
};


/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

static GRID *create(void) {

	/* allocate memory for new widget */
	GRID *new = (GRID *)mem->alloc(sizeof(GRID)+sizeof(struct widget_data));
	if (!new) {
		DOPEDEBUG(printf("Grid(create): out of memory\n"));
		return NULL;
	}

	new->gen  = &gen_methods;	/* pointer to general widget methods */
	new->grid = &grd_methods;	/* pointer to grid specific methods */

	/* set general widget attributes */
	new->wd = (struct widget_data *)((long)new + sizeof(GRID));
	widman->default_widget_data(new->wd);
	
	/* set grid specific widget attributes */
	new->wd->min_w=16;
	new->wd->min_h=16;
	new->rows=NULL;
	new->cols=NULL;
	new->cells=NULL;
	new->update=0;
	
	return new;
}


/****************************************/
/*** SERVICE STRUCTURE OF THIS MODULE ***/
/****************************************/

static struct grid_services services = {
	create
};


/**************************/
/*** MODULE ENTRY POINT ***/
/**************************/


static void script_column_config(GRID *g,long index,float weight,long width) {
	if (width!=-1) grid_set_col_w(g,index,width);
	else if (weight!=-1.0) grid_set_col_weight(g,index,weight);
	grid_update(g,1);
}

static void script_row_config(GRID *g,long index,float weight,long width) {
	if (width!=-1) grid_set_row_h(g,index,width);
	else if (weight!=-1.0) grid_set_row_weight(g,index,weight);
	grid_update(g,1);
}

static void script_place_widget(GRID *g,WIDGET *w,long col,long row,
										   long col_span,long row_span,
										   long pad_x,long pad_y) {

	/* check if widget is a child of the grid - if not, try to adopt it */
	if (!w->gen->get_parent(w)) grid_add(g,w);

	if (col!=9999) grid_set_col(g,w,col);
	if (row!=9999) grid_set_row(g,w,row);
	if (col_span!=9999) grid_set_col_span(g,w,col_span);
	if (row_span!=9999) grid_set_row_span(g,w,row_span);
	if (pad_x!=9999) grid_set_pad_x(g,w,pad_x);
	if (pad_y!=9999) grid_set_pad_y(g,w,pad_y);
	grid_update(g,1);

}


static void build_script_lang(void) {
	void *widtype;

	widtype = script->reg_widget_type("Grid",(void *(*)(void))create);

	script->reg_widget_method(widtype,"void columnconfig(long index,float weight=-1.0,long size=-1)",&script_column_config);
	script->reg_widget_method(widtype,"void rowconfig(long index,float weight=-1.0,long size=-1)",&script_row_config);
	script->reg_widget_method(widtype,"void place(Widget child,long column=9999,long row=9999,long columnspan=9999,long rowspan=9999,long padx=9999,long pady=9999)",script_place_widget);
	script->reg_widget_method(widtype,"void add(Widget child)",grid_add);
	script->reg_widget_method(widtype,"void remove(Widget child)",grid_remove);
	
	widman->build_script_lang(widtype,&gen_methods);
}


int init_grid(struct dope_services *d) {

	mem		= d->get_module("Memory 1.0");
	widman	= d->get_module("WidgetManager 1.0");
	clip	= d->get_module("Clipping 1.0");
	bg		= d->get_module("Background 1.0");
	script	= d->get_module("Script 1.0");
	
	/* define general widget functions */
	widman->default_widget_methods(&gen_methods);
	
	orig_update = gen_methods.update;
	
	gen_methods.draw	= grid_draw;
	gen_methods.find	= grid_find;
	gen_methods.update	= grid_update;

	build_script_lang();
	
	d->register_module("Grid 1.0",&services);
	return 1;
}
