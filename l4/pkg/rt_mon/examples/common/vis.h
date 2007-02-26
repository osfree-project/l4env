#ifndef __RT_MON_EXAMPLES_COMMON_VIS_H_
#define __RT_MON_EXAMPLES_COMMON_VIS_H_

#include <l4/rt_mon/types.h>

#include "wins.h"

// 0 .. 99 are valid values
#define BUTTON_RESET   0
#define BUTTON_DUMP    1
#define BUTTON_ADAPT   2
#define BUTTON_LOGMODE 3
#define BUTTON_MINUS1 10
#define BUTTON_PLUS1  11
#define BUTTON_MINUS2 12
#define BUTTON_PLUS2  13
#define BUTTON_ENTRY1 14
#define BUTTON_ENTRY2 15

#define WINDOW_TYPE_HISTO      1
#define WINDOW_TYPE_HISTO2D    2
#define WINDOW_TYPE_LIST       3
#define WINDOW_TYPE_FANCY_LIST 4
#define WINDOW_TYPE_SCALAR     5

void get_color(int val, double scale, int *r, int *g, int *b);

void vis_create(wins_t *wins);
void vis_refresh(const wins_t *wins);

void vis_create_histogram(rt_mon_histogram_t *hist, wins_t *wins);
void vis_refresh_histogram(rt_mon_histogram_t *hist, const wins_t *wins);

void vis_create_histogram2d(rt_mon_histogram2d_t *hist, wins_t *wins);
void vis_refresh_histogram2d(rt_mon_histogram2d_t *hist, const wins_t *wins);

void vis_create_events(rt_mon_event_list_t *list, wins_t *wins);
void vis_refresh_events(rt_mon_event_list_t *list, const wins_t *wins);

void vis_create_fancy_events(rt_mon_event_list_t *list, wins_t *wins);
void vis_refresh_fancy_events(rt_mon_event_list_t *list, const wins_t *wins);

void vis_create_scalar(rt_mon_scalar_t *scalar, wins_t *wins);
void vis_refresh_scalar(rt_mon_scalar_t *scalar, const wins_t *wins);

/* lock must be acquired externally
 */
void vis_close(wins_t *wins);

#endif
