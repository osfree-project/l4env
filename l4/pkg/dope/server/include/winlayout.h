#if !defined(WIDGET)
#include "widget.h"
#endif

struct winlayout_services {
	WIDGET *(*create_win_elements) (s32 elem_mask,s32 width,s32 height);
	void    (*resize_win_elements) (WIDGET *first_elem,s32 elem_mask,s32 width,s32 height);
	void    (*set_win_title)       (WIDGET *first_elem,char *new_title);
	char   *(*get_win_title)       (WIDGET *first_elem);
	void    (*set_win_state)       (WIDGET *first_elem,s32 state);
	s32     (*get_left_border)     (s32 elem_mask);
	s32     (*get_right_border)    (s32 elem_mask);
	s32     (*get_top_border)      (s32 elem_mask);
	s32     (*get_bottom_border)   (s32 elem_mask);
};
