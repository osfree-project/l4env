/*!
 * \file   rt_mon/examples/common/vis.c
 * \brief  Visualization for the various data pools
 *
 * \date   01/14/2005
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 * \author Jork Loeser     <jork.loeser@inf.tu-dresden.de>
 */
/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#define _GNU_SOURCE

#include "vis.h"
#include "wins.h"
#include "color.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>

#include <l4/dope/dopelib.h>
#include <l4/dope/vscreen.h>
#include <l4/libvfb/vfb.h>
#include <l4/log/l4log.h>

#include <l4/rt_mon/histogram.h>
#include <l4/rt_mon/histogram2d.h>
#include <l4/rt_mon/event_list.h>
#include <l4/rt_mon/l4vfs_monitor.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

extern long app_id;

typedef struct
{
    int xselection;  // last motion position
    int start;       // start of selection
    int stop;        // end of selection
    int mode;        // 0 == normal,
                     // 1 == selection started, 2 == selection stopped
    int log_mode;    // 0 == normal, 1 logarithmic mode
} hist_local_t;

extern double own_floor(double x);

void press_callback2(dope_event *e, void *arg);


static void _motion_list_callback(dope_event *ev, void *arg)
{
    fancy_list_t *local = (fancy_list_t *)((wins_t *)arg)->local;

    if (local)
    {
        local->mouse_posx = ev->motion.abs_x;
        local->mouse_posy = ev->motion.abs_y;
    }
}

static void _leave_list_callback(dope_event *ev, void *arg)
{
    fancy_list_t *local = (fancy_list_t *)((wins_t *)arg)->local;

    if (local)
    {
        local->mouse_posx = -1;
        local->mouse_posy = -1;
        local->mouse_but = 0;
    }
}

static void _button_list_callback(dope_event *ev, void *arg)
{
    fancy_list_t *local = (fancy_list_t *)((wins_t *)arg)->local;

    if (local)
    {
        if (ev->type == EVENT_TYPE_PRESS)
            local->mouse_but = 1;
        else if (ev->type == EVENT_TYPE_RELEASE)
            local->mouse_but = 0;
    }
}

static void _motion_callback(dope_event *ev, void *arg)
{
    wins_t *wins = (wins_t *)arg;
    hist_local_t *local = (hist_local_t *)wins->local;

    if (local)
    {
        local->xselection = ev->motion.abs_x;
        if (local->mode == 1)
            local->stop = ev->motion.abs_x;
    }
}

static void _leave_callback(dope_event *ev, void *arg)
{
    hist_local_t *local = (hist_local_t *)((wins_t *)arg)->local;

    if (local)
    {
        if (local->mode != 1)
            local->xselection = -1;
    }
}

static void _button_callback(dope_event *ev, void *arg)
{
    hist_local_t *local = (hist_local_t *)((wins_t *)arg)->local;

    if (local)
    {
        if (ev->type == EVENT_TYPE_PRESS)
        {
            local->mode  = 1;
            local->start = local->xselection;
            local->stop  = local->xselection;
        }
        else if (ev->type == EVENT_TYPE_RELEASE)
        {
            local->stop = local->xselection;
            if (local->stop == local->start)
                local->mode = 0;
            else
            {
                local->mode = 2;
            }
        }
    }
}

static void _commit_callback(dope_event *ev, void *arg)
{
    int id = (int)arg;
    int fd = id / BIND_BUTTON_SCALER;
    int function = id % BIND_BUTTON_SCALER;
    int win_index = wins_index_for_fd(fd);
    wins_t *win;
    char buf [100];
    long long temp;
    fancy_list_t *temp_fl;
    int ret;

    if (fd < 0)
    {
        LOG("Strange, we got called from a dead window, fd = %d!", fd);
        return;
    }

    win = &(wins[win_index]);
    if (win->type != WINDOW_TYPE_FANCY_LIST)
        return;

    switch (function)
    {
    case BUTTON_ENTRY1:
        dope_reqf(app_id, buf, sizeof(buf), "entry_%d_a.text", fd);
        break;
    case BUTTON_ENTRY2:
        dope_reqf(app_id, buf, sizeof(buf), "entry_%d_b.text", fd);
        break;
    default:
        LOG("Unknow case, ignored!");
        return;
    }
    ret = sscanf(buf, "%lld", &temp);
    if (ret != 1)
        return;

    temp_fl = (fancy_list_t *)win->local;

    switch (function)
    {
    case BUTTON_ENTRY1:
        temp_fl->high = temp;
        break;
    case BUTTON_ENTRY2:
        temp_fl->low = temp;
        break;
    }
}


void vis_create(wins_t *wins)
{
    switch(wins->type)
    {
    case WINDOW_TYPE_HISTO:
        vis_create_histogram((rt_mon_histogram_t *)wins->data, wins);
        break;

    case WINDOW_TYPE_HISTO2D:
        vis_create_histogram2d((rt_mon_histogram2d_t *)wins->data, wins);
        break;

    case WINDOW_TYPE_LIST:
        vis_create_events((rt_mon_event_list_t *)wins->data, wins);
        break;

    case WINDOW_TYPE_FANCY_LIST:
        vis_create_fancy_events((rt_mon_event_list_t *)wins->data, wins);
        break;

    case WINDOW_TYPE_SCALAR:
        vis_create_scalar((rt_mon_scalar_t *)wins->data, wins);
        break;
    }
}

void vis_refresh(const wins_t *wins)
{
    switch(wins->type)
    {
    case WINDOW_TYPE_HISTO:
        vis_refresh_histogram((rt_mon_histogram_t *)wins->data, wins);
        break;

    case WINDOW_TYPE_HISTO2D:
        vis_refresh_histogram2d((rt_mon_histogram2d_t *)wins->data, wins);
        break;

    case WINDOW_TYPE_LIST:
        vis_refresh_events((rt_mon_event_list_t *)wins->data, wins);
        break;

    case WINDOW_TYPE_FANCY_LIST:
        vis_refresh_fancy_events((rt_mon_event_list_t *)wins->data, wins);
        break;

    case WINDOW_TYPE_SCALAR:
        vis_refresh_scalar((rt_mon_scalar_t *)wins->data, wins);
        break;
    }
}


void vis_create_histogram(rt_mon_histogram_t *hist, wins_t *wins)
{
    int bins;
    char cmd[100];
    int fd = wins->fd;

    bins = hist->bins;
    dope_cmdf(app_id, "win_%d = new Window()", fd);
    dope_cmdf(app_id, "win_%d.set(-title \"%s\")", fd, hist->name);
    dope_cmdf(app_id, "vscr_%d = new VScreen()", fd);
    dope_cmdf(app_id, "vscr_%d.setmode(%d, %d, \"RGB16\")",
              fd, bins, WINDOW_H);
    dope_cmdf(app_id, "vscr_%d.set(-fixw %d -fixh %d)", fd, bins, WINDOW_H);

    // grids
    dope_cmd(app_id, "temp = new Grid()");
    dope_cmd(app_id, "temp2 = new Grid()");

    // label
    dope_cmdf(app_id, "label_%d_a = new Label()", fd);
    dope_cmdf(app_id, "label_%d_a.set(-text testing_a)", fd);
    dope_cmdf(app_id, "label_%d_b = new Label()", fd);
    dope_cmdf(app_id, "label_%d_b.set(-text testing_b)", fd);
    dope_cmdf(app_id, "label_%d_c = new Label()", fd);
    dope_cmdf(app_id, "label_%d_c.set(-text testing_c)", fd);
    dope_cmdf(app_id, "temp2.place(label_%d_a, -column 1 -row 1)", fd);
    dope_cmdf(app_id, "temp2.place(label_%d_b, -column 1 -row 2)", fd);
    dope_cmdf(app_id, "temp2.place(label_%d_c, -column 1 -row 3)", fd);
    dope_cmdf(app_id, "temp.place(temp2, -column 1 -row 1)", fd);

    // buttons
    dope_cmd(app_id, "tempb = new Button()");
    dope_cmd(app_id, "tempb.set(-text Reset)");
    dope_cmdf(app_id, "temp2.place(tempb, -column 2 -row 1)");
    dope_bind(app_id, "tempb", "clack", press_callback2,
              (void *)(fd * BIND_BUTTON_SCALER + BUTTON_RESET));

    dope_cmd(app_id, "tempb = new Button()");
    dope_cmd(app_id, "tempb.set(-text Dump)");
    dope_cmdf(app_id, "temp2.place(tempb, -column 2 -row 2)");
    dope_bind(app_id, "tempb", "clack", press_callback2,
              (void *)(fd * BIND_BUTTON_SCALER + BUTTON_DUMP));

    dope_cmd(app_id, "tempg = new Grid()");
    dope_cmdf(app_id, "temp2.place(tempg, -column 2 -row 3)");

    dope_cmd(app_id, "tempb = new Button()");
    dope_cmd(app_id, "tempb.set(-text Adapt)");
    dope_cmdf(app_id, "tempg.place(tempb, -column 1 -row 1)");
    dope_bind(app_id, "tempb", "clack", press_callback2,
              (void *)(fd * BIND_BUTTON_SCALER + BUTTON_ADAPT));

    dope_cmdf(app_id, "tempblog%d = new Button()", fd);
    dope_cmdf(app_id, "tempblog%d.set(-text Lin)", fd);
    dope_cmdf(app_id, "tempg.place(tempblog%d, -column 2 -row 1)", fd);
    dope_bindf(app_id, "tempblog%d", "clack", press_callback2,
	       (void *)(fd * BIND_BUTTON_SCALER + BUTTON_LOGMODE), fd);

    // motion stuff
    wins->local = malloc(sizeof(hist_local_t));
    if (wins->local)
    {
        ((hist_local_t*)wins->local)->xselection = -1;
        ((hist_local_t*)wins->local)->start = 0;
        ((hist_local_t*)wins->local)->stop = 0;
        ((hist_local_t*)wins->local)->mode = 0;
        ((hist_local_t*)wins->local)->log_mode = 0;
    }
    dope_bindf(app_id, "vscr_%d", "motion", _motion_callback, wins, fd);
    dope_bindf(app_id, "vscr_%d", "press", _button_callback, wins, fd);
    dope_bindf(app_id, "vscr_%d", "release", _button_callback, wins, fd);
    dope_bindf(app_id, "vscr_%d", "leave", _leave_callback, wins, fd);

    // connect
    dope_cmdf(app_id, "temp.place(vscr_%d, -column 1 -row 2)", fd);
    dope_cmdf(app_id, "win_%d.set(-x 300 -y 20 -content temp)", fd);
    dope_cmdf(app_id, "win_%d.open()", fd);

    snprintf(cmd, 100, "vscr_%d", fd);
    wins->vscr = vscr_get_fb(app_id, cmd);
    if (wins->vscr == NULL)
        LOG("Could not map vscreen!");

    wins->w = bins;
    wins->h = WINDOW_H;
}

void vis_refresh_histogram(rt_mon_histogram_t *hist, const wins_t *wins)
{
    int max, x, y, xselection = -1, yselection = -1,
        mode = 0, start = 0, stop = 0;
    double scale;
    int fd = wins->fd;
    hist_local_t * local;

    if(wins->local)
    {
	local = (hist_local_t *) wins->local;
    }
    else
    {
	local = 0;
    }
    // compute scale
    for (x = 0, max = 0; x < wins->w; x++)
        max = MAX(max, hist->data[x]);
    if(local && local->log_mode)
    {
	scale = MAX(1.0 / WINDOW_H, (log(max + 2.0)) / wins->h);
    }
    else
    {
	scale = MAX(1.0 / WINDOW_H, (max + 2.0) / wins->h);
    }
    if (local)
    {
        xselection = ((hist_local_t *)(wins->local))->xselection;
	/* snap in xselection to our values, wich are integers */
	if (xselection >= 0 && hist->bin_size < 1)
        {
	    xselection = own_floor((own_floor(xselection *
					      hist->bin_size + hist->low + .5)
				    - hist->low) / hist->bin_size);
	    if (xselection < 0)
                xselection = 0;
	    if (xselection >= wins->w)
                xselection = wins->w - 1;
	}

        mode       = ((hist_local_t *)(wins->local))->mode;
        start      = ((hist_local_t *)(wins->local))->start;
        stop       = ((hist_local_t *)(wins->local))->stop;
        if (start > stop)
        {
            int temp;
            temp  = start;
            start = stop;
            stop  = temp;
        }
    }

    // draw
    vfb_clear_screen(wins->vscr, wins->w, wins->h);

    // highlight selection
    if (mode > 0 && wins->local)
    {
        vfb_fill_rect_rgb(wins->vscr, wins->w, wins->h,
                          start, 0, stop, wins->h - 1,
                          0, 0, 255);
    }
    // draw histogram data itself and current mouse pointer
    for (x = 0; x < wins->w; x++)
    {
	if(local && local->log_mode)
        {
	    y = hist->data[x] ? log(hist->data[x]) / scale : 0;
	    scale = MAX(1.0 / WINDOW_H, (log(max + 2.0)) / wins->h);
	}
	else
	{
	    y = hist->data[x] / scale;
	}
	
        if (xselection == x)
        {
            vfb_vbar_rgb(wins->vscr, wins->w, wins->h, x, 0, wins->h - y - 1,
                         255, 255, 255);
            vfb_vbar_rgb(wins->vscr, wins->w, wins->h, x, wins->h - y, y - 1,
                         255, 0, 128);
            yselection = y;
        }
        else
        {
            vfb_vbar_rgb_f(wins->vscr, wins->w, wins->h, x, wins->h - y, y - 1,
                           255, 255, 128);
        }
    }
    if (yselection >= 0)
    {
        vfb_hbar_rgb(wins->vscr, wins->w, wins->h, 0,
                     wins->h - yselection, wins->w - 1,
                     255, 255, 255);
    }

    // paint a grind above all
    vfb_draw_grid(wins->vscr, wins->w, wins->h, 4, 4, 4, 4,
                  50, 255, 100, 50, 100, 255);

    // refresh text strings
    dope_cmdf(app_id, "label_%d_a.set(-text \""
              "[%g, %g) %s->%s, bins = %d\")",
              fd, hist->low, hist->high, hist->unit[0],
              hist->unit[1], hist->bins);
    if (yselection >= 0 && mode == 0)
    {
        long long val;
	unsigned long long qsum=0, sum=0;
	int i;

        val = own_floor(hist->low + xselection * hist->bin_size + .5);

	/* get quantile of the selection with regard to values */
	for(i = 0; i < wins->w; i++)
	{
	    sum += hist->data[i];
	    if(i <= xselection) qsum += hist->data[i];
	}

        dope_cmdf(app_id,
		  "label_%d_b.set(-text \"%lld %s -> %d %s, %d%%-quantile\")",
		  fd,
                  val, hist->unit[0], hist->data[xselection], hist->unit[1],
		  (int)(sum ? (qsum * 100) / sum : 0));
    }
    else if (mode > 0)
    {
        long long val1, val2;

        val1 = own_floor(hist->low + start * hist->bin_size + .5);
        val2 = own_floor(hist->low + stop * hist->bin_size + .5);
        dope_cmdf(app_id, "label_%d_b.set(-text \"[%lld, %lld) %s\")", fd,
                  val1, val2, hist->unit[0]);
    }
    else
    {
        dope_cmdf(app_id, "label_%d_b.set(-text \""
                  "udfl = %u, ovfl = %u, ovh = %u, scl = %g\")", fd,
                  hist->underflow, hist->overflow, hist->time_ovh,
                  scale * WINDOW_H);
    }
    dope_cmdf(app_id, "label_%d_c.set(-text \""
              "%d samples, (%lld ... %lld), avg %lld, lost %d\")", fd,
              (int)hist->val_count,
              (hist->val_min == LLONG_MAX ? 0LL : hist->val_min),
              (hist->val_max == LLONG_MIN ? 0LL : hist->val_max),
              (hist->val_count ? (hist->val_sum / hist->val_count) : 0),
	      hist->lost);
    dope_cmdf(app_id, "vscr_%d.refresh()", fd);
}

void vis_create_histogram2d(rt_mon_histogram2d_t *hist, wins_t *wins)
{
    int bins[2], x, y, ret;
    char cmd[100];
    l4_int16_t *ck;
    int fd = wins->fd;

    bins[0] = hist->bins[0];
    bins[1] = hist->bins[1];
    dope_cmdf(app_id, "win_%d = new Window()", fd);
    dope_cmdf(app_id, "win_%d.set(-title \"%s\")", fd, hist->name);
    dope_cmdf(app_id, "vscr_%d = new VScreen()", fd);
    dope_cmdf(app_id, "vscr_%d.setmode(%d, %d, \"RGB16\")",
              fd, bins[0], bins[1]);
    dope_cmdf(app_id, "vscr_%d.set(-fixw %d -fixh %d)",
              fd, bins[0], bins[1]);
    // grids
    dope_cmd(app_id, "temp = new Grid()");
    dope_cmd(app_id, "temp2 = new Grid()");
    // color key
    dope_cmd(app_id, "scale = new VScreen()");
    dope_cmd(app_id, "scale.setmode(256, 10, \"RGB16\")");
    dope_cmd(app_id, "scale.set(-fixw 256 -fixh 10)");
    dope_cmd(app_id, "temp.place(scale, -column 1 -row 3)");
    ck = vscr_get_fb(app_id, "scale");
    // draw color key
    for (x = 0; x < 256; x++)
        for (y = 0; y < 10; y++)
        {
            int r, g, b;
            get_color_for_val(x, 1, &r, &g, &b);
            vfb_setpixel_rgb(ck, 256, 10, x, y, r, g, b);
        }
    ret = vscr_free_fb(ck);
    if (ret)
    {
        LOG("Problem freeing vscreen framebuffer: ret = %d", ret);
    }

    // label
    dope_cmdf(app_id, "label_%d_a = new Label()", fd);
    dope_cmdf(app_id, "label_%d_a.set(-text testing)", fd);
    dope_cmdf(app_id, "label_%d_b = new Label()", fd);
    dope_cmdf(app_id, "label_%d_b.set(-text testing)", fd);
    dope_cmdf(app_id, "temp2.place(label_%d_a, -column 1 -row 1)", fd);
    dope_cmdf(app_id, "temp2.place(label_%d_b, -column 1 -row 2)", fd);
    dope_cmdf(app_id, "temp.place(temp2, -column 1 -row 1)", fd);
    // buttons
    dope_cmd(app_id, "tempb = new Button()");
    dope_cmd(app_id, "tempb.set(-text Reset)");
    dope_cmdf(app_id, "temp2.place(tempb, -column 2 -row 1)");
    dope_bind(app_id, "tempb", "clack", press_callback2,
              (void *)(fd * BIND_BUTTON_SCALER + BUTTON_RESET));
    dope_cmd(app_id, "tempb = new Button()");
    dope_cmd(app_id, "tempb.set(-text Dump)");
    dope_cmdf(app_id, "temp2.place(tempb, -column 2 -row 2)");
    dope_bind(app_id, "tempb", "clack", press_callback2,
              (void *)(fd * BIND_BUTTON_SCALER + BUTTON_DUMP));

    dope_cmdf(app_id, "temp.place(vscr_%d, -column 1 -row 2)", fd);
    dope_cmdf(app_id, "win_%d.set(-x 300 -y 20 -content temp)", fd);
    dope_cmdf(app_id, "win_%d.open()", fd);

    snprintf(cmd, 100, "vscr_%d", fd);
    wins->vscr = vscr_get_fb(app_id, cmd);
    if (wins->vscr == NULL)
        LOG("Could not map vscreen!");

    wins->w = bins[0];
    wins->h = bins[1];
}

void vis_refresh_histogram2d(rt_mon_histogram2d_t *hist, const wins_t *wins)
{
    int x, y;
    unsigned int max;
    double scale;
    int fd = wins->fd;

    // draw
    vfb_clear_screen(wins->vscr, wins->w, wins->h);
    if (hist->layers == 1)
    {
        // compute scale
        for (x = 0, max = 0; x < wins->w * wins->h; x++)
        {
            max = MAX(max, hist->data[x]);
        }
        scale = MAX(1.0 / WINDOW_H, (max + 1.0) / 256);
        for (y = 0; y < wins->h; y++)
            for (x = 0; x < wins->w; x++)
            {
                int r, g, b;
                get_color_for_val(hist->data[y * wins->w + x], scale,
                                  &r, &g, &b);
                vfb_setpixel_rgb(wins->vscr,
                                 wins->w, wins->h,
                                 x, wins->h - y - 1, r, g, b);
            }
    }
    else //if (hist->layers > 1)
    {
        // compute scale
        for (x = 0, max = 0; x < wins->w * wins->h; x++)
        {
            unsigned int val, div;
            val = hist->data[x];
            div = hist->data[x + wins->w * wins->h];
            if (val)
            {
                max = MAX(max, val / div);
            }
        }
        scale = MAX(1.0 / WINDOW_H, (max + 1.0) / 256);
        for (y = 0; y < wins->h; y++)
            for (x = 0; x < wins->w; x++)
            {
                int r, g, b, val, count;
                val   = hist->data[y * hist->bins[0] + x];
                count = hist->data[1 * hist->bins[0] *
                                   hist->bins[1] +
                                   y * hist->bins[0] + x];
                if (count > 0)
                    get_color_for_val(val / count, scale, &r, &g, &b);
                else
                    get_color_for_val(0, scale, &r, &g, &b);
                vfb_setpixel_rgb(wins->vscr,
                                 wins->w, wins->h,
                                 x, wins->h - y - 1, r, g, b);
            }
    }

    // refresh
    dope_cmdf(app_id, "label_%d_a.set(-text \""
              "[<%g, %g>-<%g, %g>) <%s, %s>->%s, "
              "bins: <%d, %d>\")", fd,
              hist->low[0], hist->low[1],
              hist->high[0], hist->high[1],
              hist->unit[0], hist->unit[1], hist->unit[2],
              hist->bins[0], hist->bins[1]);
    dope_cmdf(app_id, "label_%d_b.set(-text \""
              "l: %d, ud-/ovfl: <%u, %u>, ovh = %u, "
              "scl = %g\")", fd, hist->layers,
              hist->underflow, hist->overflow, hist->time_ovh,
              scale * WINDOW_H);
    dope_cmdf(app_id, "vscr_%d.refresh()", fd);

}

void vis_create_events(rt_mon_event_list_t *list, wins_t *wins)
{
    char cmd[100];
    ring_buf_t *temp;
    int fd = wins->fd;

    dope_cmdf(app_id, "win_%d = new Window()", fd);
    dope_cmdf(app_id, "win_%d.set(-title \"%s\")", fd, list->name);
    dope_cmdf(app_id, "vscr_%d = new VScreen()", fd);
    dope_cmdf(app_id, "vscr_%d.setmode(%d, %d, \"RGB16\")",
              fd, WINDOW_LIST_W, WINDOW_H);
    dope_cmdf(app_id, "vscr_%d.set(-fixw %d -fixh %d)", fd, WINDOW_LIST_W,
              WINDOW_H);
    // grids
    dope_cmd(app_id, "temp = new Grid()");
    dope_cmd(app_id, "temp2 = new Grid()");
    // label
    dope_cmdf(app_id, "label_%d = new Label()", fd);
    dope_cmdf(app_id, "label_%d.set(-text testing)", fd);
    dope_cmdf(app_id, "temp2.place(label_%d, -column 1 -row 1)", fd);
    dope_cmdf(app_id, "temp.place(temp2, -column 1 -row 1)", fd);
    // buttons
    dope_cmd(app_id, "tempb = new Button()");
    dope_cmd(app_id, "tempb.set(-text Reset)");
    dope_cmdf(app_id, "temp2.place(tempb, -column 2 -row 1)");
    dope_bind(app_id, "tempb", "clack", press_callback2,
              (void *)(fd * BIND_BUTTON_SCALER + BUTTON_RESET));
    dope_cmd(app_id, "tempb = new Button()");
    dope_cmd(app_id, "tempb.set(-text Dump)");
    dope_cmdf(app_id, "temp2.place(tempb, -column 3 -row 1)");
    dope_bind(app_id, "tempb", "clack", press_callback2,
              (void *)(fd * BIND_BUTTON_SCALER + BUTTON_DUMP));

    dope_cmdf(app_id, "temp.place(vscr_%d, -column 1 -row 2)", fd);
    dope_cmdf(app_id, "win_%d.set(-x 300 -y 220 -content temp)", fd);
    dope_cmdf(app_id, "win_%d.open()", fd);

    snprintf(cmd, 100, "vscr_%d", fd);
    wins->vscr = vscr_get_fb(app_id, cmd);
    if (wins->vscr == NULL)
        LOG("Could not map vscreen!");

    wins->w = WINDOW_LIST_W;
    wins->h = WINDOW_H;
    // setup some local data structures
    temp = malloc(sizeof(ring_buf_t) +
                  BUFFER_LIST * sizeof(temp->data[0]));
    temp->index = 0;
    temp->max   = BUFFER_LIST;
    bzero(&(temp->data[0]), BUFFER_LIST * sizeof(temp->data[0]));
    wins->local = temp;
}

void vis_refresh_events(rt_mon_event_list_t *list, const wins_t *wins)
{
    int max, scale, x, y, index, ret, avg, avg_count;
    rt_mon_basic_event_t ev;
    ring_buf_t *rb;
    int fd = wins->fd;

    rb = (ring_buf_t *)wins->local;

    // transfer data from ds to local buffer
    if (list->event_type == RT_MON_EVTYPE_BASIC)
    {
        do
        {
            ret = rt_mon_list_remove(list, &ev);
            if (ret == 0)
            {
                rb->data[rb->index] = ev.time;
                rb->index = (rb->index + 1) % rb->max;
            }
        }
        while (ret == 0);
    }
    else
        return;

    // compute scale
    max = 0;
    for (x = 0, index = (rb->index - 1 + rb->max) % rb->max;
         x < wins->w;
         x++, index = (index - 1 + rb->max) % rb->max)
    {
        max = MAX(max, rb->data[index]);
    }
    scale = MAX(1, max / wins->h + 1);

    // draw
    avg = 0;
    avg_count = 0;
    vfb_clear_screen(wins->vscr, wins->w, wins->h);
    for (x = wins->w - 1,
             index = (rb->index - 1 + rb->max) % rb->max;
         x >= 0;
         x--, index = (index - 1 + rb->max) % rb->max)
    {
        y = rb->data[index] / scale;
        vfb_vbar_rgb_f(wins->vscr, wins->w, wins->h, x, wins->h - y, y - 1,
                       255, 255, 128);
        if (wins->w - x < 50)
        {
            avg += rb->data[index];
            avg_count++;
        }
    }
    vfb_hbar_rgb(wins->vscr, wins->w, wins->h,
                 0, wins->h - (avg / (avg_count * scale)), wins->w,
                 255, 0, 0);

    // refresh
    dope_cmdf(app_id, "label_%d.set(-text \""
              "size = %d, overruns = %d, scale = %d\")",
              fd, wins->w, list->overruns, scale);
    dope_cmdf(app_id, "vscr_%d.refresh()", fd);
}

void vis_create_fancy_events(rt_mon_event_list_t *list, wins_t *wins)
{
    char cmd[100];
    int fd = wins->fd;

    dope_cmdf(app_id, "win_%d = new Window()", fd);
    dope_cmdf(app_id, "win_%d.set(-title \"%s\")", fd, list->name);
    dope_cmdf(app_id, "vscr_%d = new VScreen()", fd);
    dope_cmdf(app_id, "vscr_%d.setmode(%d, %d, \"RGB16\")",
              fd, WINDOW_LIST_W, WINDOW_H);
    dope_cmdf(app_id, "vscr_%d.set(-fixw %d -fixh %d)", fd, WINDOW_LIST_W,
              WINDOW_H);
    dope_bindf(app_id, "vscr_%d", "motion", _motion_list_callback, wins, fd);
    dope_bindf(app_id, "vscr_%d", "leave", _leave_list_callback, wins, fd);
    dope_bindf(app_id, "vscr_%d", "press", _button_list_callback, wins, fd);
    dope_bindf(app_id, "vscr_%d", "release", _button_list_callback, wins, fd);

    // grids
    dope_cmd(app_id, "temp = new Grid()");
    dope_cmdf(app_id, "temp.place(vscr_%d, -column 1 -row 2)", fd);
    dope_cmd(app_id, "temp2 = new Grid()");
    dope_cmd(app_id, "temp2.columnconfig(0, -size 20)");
    dope_cmd(app_id, "temp2.columnconfig(1, -size 60)");
    dope_cmd(app_id, "temp2.columnconfig(2, -size 20)");
    dope_cmd(app_id, "temp2.columnconfig(4, -size 50)");
    dope_cmdf(app_id, "temp.place(temp2, -column 1 -row 1)", fd);
    dope_cmdf(app_id, "win_%d.set(-x 300 -y 220 -content temp)", fd);

    // buttons
    dope_cmd(app_id, "tempb = new Button()");
    dope_cmd(app_id, "tempb.set(-text Reset)");
    dope_cmdf(app_id, "temp2.place(tempb, -column 4 -row 0)");
    dope_bind(app_id, "tempb", "clack", press_callback2,
              (void *)(fd * BIND_BUTTON_SCALER + BUTTON_RESET));
    dope_cmd(app_id, "tempb = new Button()");
    dope_cmd(app_id, "tempb.set(-text Dump)");
    dope_cmdf(app_id, "temp2.place(tempb, -column 4 -row 1)");
    dope_bind(app_id, "tempb", "clack", press_callback2,
              (void *)(fd * BIND_BUTTON_SCALER + BUTTON_DUMP));

    // label
    dope_cmdf(app_id, "label_%d_a = new Label()", fd);
    dope_cmdf(app_id, "label_%d_a.set(-text testinga)", fd);
    dope_cmdf(app_id, "temp2.place(label_%d_a, -column 3 -row 0)", fd);
    dope_cmdf(app_id, "label_%d_b = new Label()", fd);
    dope_cmdf(app_id, "label_%d_b.set(-text testingb)", fd);
    dope_cmdf(app_id, "temp2.place(label_%d_b, -column 3 -row 1)", fd);

    // modifiers
    dope_cmd(app_id, "tempb = new Button()");
    dope_cmd(app_id, "tempb.set(-text \"-\")");
    dope_cmdf(app_id, "temp2.place(tempb, -column 0 -row 0)");
    dope_bind(app_id, "tempb", "clack", press_callback2,
              (void *)(fd * BIND_BUTTON_SCALER + BUTTON_MINUS1));
    dope_cmd(app_id, "tempb = new Button()");
    dope_cmd(app_id, "tempb.set(-text \"-\")");
    dope_cmdf(app_id, "temp2.place(tempb, -column 0 -row 1)");
    dope_bind(app_id, "tempb", "clack", press_callback2,
              (void *)(fd * BIND_BUTTON_SCALER + BUTTON_MINUS2));
    dope_cmd(app_id, "tempb = new Button()");
    dope_cmd(app_id, "tempb.set(-text \"+\")");
    dope_cmdf(app_id, "temp2.place(tempb, -column 2 -row 0)");
    dope_bind(app_id, "tempb", "clack", press_callback2,
              (void *)(fd * BIND_BUTTON_SCALER + BUTTON_PLUS1));
    dope_cmd(app_id, "tempb = new Button()");
    dope_cmd(app_id, "tempb.set(-text \"+\")");
    dope_cmdf(app_id, "temp2.place(tempb, -column 2 -row 1)");
    dope_bind(app_id, "tempb", "clack", press_callback2,
              (void *)(fd * BIND_BUTTON_SCALER + BUTTON_PLUS2));

    // entry fields
    dope_cmdf(app_id, "entry_%d_a = new Entry()", fd);
    dope_cmdf(app_id, "temp2.place(entry_%d_a, -column 1 -row 0)", fd);
    dope_bindf(app_id, "entry_%d_a", "commit", _commit_callback,
               (void *)(fd * BIND_BUTTON_SCALER + BUTTON_ENTRY1), fd);
    dope_cmdf(app_id, "entry_%d_b = new Entry()", fd);
    dope_cmdf(app_id, "temp2.place(entry_%d_b, -column 1 -row 1)", fd);
    dope_bindf(app_id, "entry_%d_b", "commit", _commit_callback,
               (void *)(fd * BIND_BUTTON_SCALER + BUTTON_ENTRY2), fd);

    dope_cmdf(app_id, "win_%d.open()", fd);

    snprintf(cmd, 100, "vscr_%d", fd);
    wins->vscr = vscr_get_fb(app_id, cmd);
    if (wins->vscr == NULL)
        LOG("Could not map vscreen!");

    wins->w = WINDOW_LIST_W;
    wins->h = WINDOW_H;
    {
        fancy_list_t *temp_fl;

        // setup some local data structures
        temp_fl = malloc(sizeof(fancy_list_t));
        temp_fl->low = 0;
        temp_fl->high = 1000;
        dope_cmdf(app_id, "entry_%d_a.set(-text \"%lld\")", fd, temp_fl->high);
        dope_cmdf(app_id, "entry_%d_b.set(-text \"%lld\")", fd, temp_fl->low);
        temp_fl->min = LLONG_MAX;
        temp_fl->max = LLONG_MIN;
        temp_fl->avg = 0;
        temp_fl->mouse_posx = -1;
        temp_fl->mouse_posy = -1;
        temp_fl->mouse_but = 0;

        temp_fl->vals = malloc(sizeof(ring_buf_t) +
                               BUFFER_LIST * sizeof(temp_fl->vals->data[0]));
        temp_fl->vals->index = 0;
        temp_fl->vals->max   = BUFFER_LIST;
        bzero(&(temp_fl->vals->data[0]),
              BUFFER_LIST * sizeof(temp_fl->vals->data[0]));

        temp_fl->avgs  = malloc(sizeof(ring_buf_t) +
                               BUFFER_LIST * sizeof(temp_fl->vals->data[0]));
        temp_fl->avgs->index = 0;
        temp_fl->avgs->max   = BUFFER_LIST;
        bzero(&(temp_fl->avgs->data[0]),
              BUFFER_LIST * sizeof(temp_fl->avgs->data[0]));

        wins->local = temp_fl;
    }
}

void vis_refresh_fancy_events(rt_mon_event_list_t *list, const wins_t *wins)
{
    int x, y1, y2, y1_old, y2_old, index, ret;
    rt_mon_basic_event_t ev;
    fancy_list_t *fl;
    int fd = wins->fd;
    int offset;
    double scale;

    fl = (fancy_list_t *)wins->local;

    // transfer data from ds to local buffer
    if (list->event_type == RT_MON_EVTYPE_BASIC)
    {
        do
        {
            ret = rt_mon_list_remove(list, &ev);
            if (ret == 0)
            {
                fl->vals->data[fl->vals->index] = ev.time;
                fl->vals->index = (fl->vals->index + 1) % fl->vals->max;
                fl->min = MIN(fl->min, ev.time);
                fl->max = MAX(fl->max, ev.time);

                fl->avg = (1 - 0.03) * fl->avg + (0.03) * ev.time;
                fl->avgs->data[fl->avgs->index] = fl->avg + 0.5;
                fl->avgs->index = (fl->avgs->index + 1) % fl->avgs->max;
            }
        }
        while (ret == 0);
    }
    else
        return;

    // compute scaler and offset for display of values from low and high
    offset = -1 * (fl->low);
    scale  = 100.0 / (fl->high - fl->low);

    vfb_clear_screen(wins->vscr, wins->w, wins->h);

    // refresh strings
    dope_cmdf(app_id, "label_%d_a.set(-text \""
              "<min, max>  <%lld, %lld>\")", fd, fl->min, fl->max);
    if (fl->mouse_posx < wins->w && fl->mouse_posx >= 0 &&
        fl->mouse_posy < wins->h && fl->mouse_posy >= 0)
    {
        if (fl->mouse_but == 0)
        {
            int i, y;

            i = (fl->vals->index - wins->w + fl->mouse_posx + fl->vals->max) %
                fl->vals->max;
            dope_cmdf(app_id, "label_%d_b.set(-text \""
                      "val = %lld, avg = %lld\")", fd,
                      fl->vals->data[i], fl->avgs->data[i]);
            // draw horizontal average line
            y = MAX(0, MIN(wins->h,
                           (int)((fl->avgs->data[i] + offset) * scale)));
            vfb_hbar_rgb(wins->vscr, wins->w, wins->h,
                         0, wins->h - y, wins->w,
                         128, 0, 0);

            // draw horizontal value line (possibly above average)
            y = MAX(0, MIN(wins->h,
                           (int)((fl->vals->data[i] + offset) * scale)));
            vfb_hbar_rgb(wins->vscr, wins->w, wins->h,
                         0, wins->h - y, wins->w,
                         128, 255, 128);
        }
        else
        {
            dope_cmdf(app_id, "label_%d_b.set(-text \""
                      "hval = %g\")", fd,
                      ((double)(wins->h - fl->mouse_posy)) / scale - offset);
            vfb_hbar_rgb(wins->vscr, wins->w, wins->h,
                         0, fl->mouse_posy, wins->w,
                         128, 128, 255);
        }
    }
    else
        dope_cmdf(app_id, "label_%d_b.set(-text \""
                  "size = %d, overruns = %d\")", fd, wins->w, list->overruns);

    // draw data
    y1_old = 0;
    y2_old = 0;
    for (x = wins->w - 1,
             index = (fl->vals->index - 1 + fl->vals->max) % fl->vals->max;
         x >= 0;
         x--, index = (index - 1 + fl->vals->max) % fl->vals->max)
    {
        // values
        y1 = MAX(0, MIN(
                     wins->h, (int)((fl->vals->data[index] + offset) * scale)));
        vfb_vbar_rgb_f(wins->vscr, wins->w, wins->h,
                       x, wins->h - y1, y1 - y1_old,
                       255, 255, 128);
        // averages
        y1_old = y1;

        y2 = MAX(0, MIN(
                     wins->h, (int)((fl->avgs->data[index] + offset) * scale)));
        vfb_vbar_rgb_f(wins->vscr, wins->w, wins->h,
                       x, wins->h - y2, y2 - y2_old,
                       255, 0, 0);
        y2_old = y2;
    }

    dope_cmdf(app_id, "vscr_%d.refresh()", fd);
}

void vis_create_scalar(rt_mon_scalar_t *scalar, wins_t *wins)
{
    int fd = wins->fd;

    dope_cmdf(app_id, "win_%d = new Window()", fd);
    dope_cmdf(app_id, "win_%d.set(-title \"%s\")", fd, scalar->name);

    // grids
    dope_cmd(app_id, "temp = new Grid()");
    dope_cmd(app_id, "temp.columnconfig(0, -size 150)");

    // scaler
    dope_cmdf(app_id, "disp_%d = new LoadDisplay()", fd);
    dope_cmdf(app_id, "disp_%d.set(-orient horizontal)", fd);
    dope_cmdf(app_id, "temp.place(disp_%d, -column 0 -row 0)", fd);

    // label
    dope_cmdf(app_id, "label_%d = new Label()", fd);
    dope_cmdf(app_id, "label_%d.set(-text testing)", fd);
    dope_cmdf(app_id, "temp.place(label_%d, -column 0 -row 1)", fd);

    // connect
    dope_cmdf(app_id, "win_%d.set(-x 300 -y 500 -content temp)", fd);
    dope_cmdf(app_id, "win_%d.open()", fd);

    wins->vscr = NULL;
}

void vis_refresh_scalar(rt_mon_scalar_t *scalar, const wins_t *wins)
{
    int fd = wins->fd;

    /* 1. reset scale dims
     * 2. set scale value
     * 3. update label
     */

    dope_cmdf(app_id, "disp_%d.set(-from %lld -to %lld)", fd,
              scalar->low, scalar->high);
    dope_cmdf(app_id, "disp_%d.barconfig(load, -value %lld)", fd,
              scalar->data);
    dope_cmdf(app_id, "label_%d.set(-text \"%lld/%lld %s\")", fd,
              scalar->data, scalar->high, scalar->unit);

    return;
}

void vis_close(wins_t *wins)
{
    int ret;
    int fd = wins->fd;

    if (wins->type == WINDOW_TYPE_FANCY_LIST)
    {
        fancy_list_t * fl;

        fl = (fancy_list_t *)wins->local;
        free(fl->vals);
        free(fl->avgs);
    }
    free(wins->local);
    wins->local = 0;

    dope_cmdf(app_id, "win_%d.close()", fd);
    if (wins->vscr != NULL)
    {
        ret = vscr_free_fb(wins->vscr);
        if (ret)
        {
            LOG("Could not free framebuffer, ignored!");
        }
    }

    wins->fd = -1;
    wins->id = -1;
}

/* Handles click events inside the monitor windows (e.g. reset, dump).
 */
void press_callback2(dope_event *e, void *arg)
{
    int id = (int)arg;
    int fd = id / BIND_BUTTON_SCALER;
    int function = id % BIND_BUTTON_SCALER;
    int win_index = wins_index_for_fd(fd);
    wins_t *win;

    if (fd < 0)
    {
        LOG("Strange, we got called from a dead window, fd = %d!", fd);
        return;
    }

    win = &(wins[win_index]);

    switch (function)
    {
    case BUTTON_LOGMODE:  // switch normal/logarithmic mode
    {
        if (win->type == WINDOW_TYPE_HISTO)
        {
            hist_local_t * local;

            local = (hist_local_t *)win->local;

            l4semaphore_down(&(win->sem));

	    local->log_mode ^= 1;

	    dope_cmdf(app_id, "tempblog%d.set(-text %s)",
		      win->fd, local->log_mode ? "Log" : "Lin");

            l4semaphore_up(&(win->sem));
        }
	break;
    }
    case BUTTON_ADAPT:  // adapt
    {
        if (win->type == WINDOW_TYPE_HISTO)
        {
            rt_mon_histogram_t * hist;
            hist_local_t * local;

            hist = (rt_mon_histogram_t *)win->data;
            local = (hist_local_t *)win->local;

            l4semaphore_down(&(win->sem));

            if (local && local->mode == 2)
            {   // adapt histogram to selection
                int start, stop;
                double temp;

                start      = local->start;
                stop       = local->stop;
                if (start > stop)
                {
                    int temp;
                    temp  = start;
                    start = stop;
                    stop  = temp;
                }

                local->mode = 0;

                temp           = own_floor(hist->low +
					   start * hist->bin_size + .5);
                hist->high     = own_floor(hist->low +
					   stop * hist->bin_size + .5);
                hist->low      = temp;
                hist->bin_size = (double)(hist->high - hist->low) /
                                         hist->bins;
            }
            else
            {   // adapt hist, if necessary
                if(hist->val_min < hist->val_max)
                {
                    hist->low      = hist->val_min;
                    hist->high     = hist->val_max;
                    hist->bin_size = (double)(hist->val_max - hist->val_min) /
                                             hist->bins;
                }
            }
            l4semaphore_up(&(win->sem));
        }
        /* falltrough to reset */
        else
        {
            break;
        }
    }
    case BUTTON_RESET:  // reset
    {
        switch (win->type)
        {
        case WINDOW_TYPE_HISTO:
        {
            l4semaphore_down(&(win->sem));
            rt_mon_hist_reset((rt_mon_histogram_t *)win->data);
            l4semaphore_up(&(win->sem));
            break;
        }
        case WINDOW_TYPE_HISTO2D:
        {
            l4semaphore_down(&(win->sem));
            rt_mon_hist2d_reset((rt_mon_histogram2d_t *)win->data);
            l4semaphore_up(&(win->sem));
            break;
        }
        case WINDOW_TYPE_LIST:
        {
            int x;
            rt_mon_event_list_t * list;
            ring_buf_t *rb;
            list = (rt_mon_event_list_t *)win->data;
            rb = (ring_buf_t *)win->local;

            l4semaphore_down(&(win->sem));
            // clear local buffer
            for (x = 0; x < rb->max; x++)
            {
                rb->data[x] = 0;
            }
            rb->index = 0;
            l4semaphore_up(&(win->sem));
            break;
        }
        case WINDOW_TYPE_FANCY_LIST:
        {
            int x;
            rt_mon_event_list_t * list;
            fancy_list_t * fl;

            list = (rt_mon_event_list_t *)win->data;

            fl = (fancy_list_t *)win->local;

            l4semaphore_down(&(win->sem));
            // clear local buffer
            for (x = 0; x < fl->vals->max; x++)
            {
                fl->vals->data[x] = 0;
                fl->avgs->data[x] = 0;
            }
            fl->vals->index = 0;
            fl->avgs->index = 0;
            fl->low  = 0;
            fl->high = 1000;
            dope_cmdf(app_id, "entry_%d_a.set(-text \"%lld\")", fd, fl->high);
            dope_cmdf(app_id, "entry_%d_b.set(-text \"%lld\")", fd, fl->low);
            fl->min  = LLONG_MAX;
            fl->max  = LLONG_MIN;
            fl->avg  = 0;
            l4semaphore_up(&(win->sem));
            break;
        }
        }
        break;
    }
    case BUTTON_DUMP:  // dump
    {
        switch (win->type)
        {
        case WINDOW_TYPE_HISTO:
        {
            l4semaphore_down(&(win->sem));
            rt_mon_hist_dump(win->data);
            l4semaphore_up(&(win->sem));
            break;
        }
        case WINDOW_TYPE_HISTO2D:
        {
            l4semaphore_down(&(win->sem));
            rt_mon_hist2d_dump(win->data);
            l4semaphore_up(&(win->sem));
            break;
        }
        case WINDOW_TYPE_LIST:
        {
            int x, index;
            rt_mon_event_list_t * list;
            ring_buf_t *rb;
            list = (rt_mon_event_list_t *)win->data;
            rb = (ring_buf_t *)win->local;

            l4semaphore_down(&(win->sem));
            for (x = 0, index = (rb->index - 1 + rb->max) % rb->max;
                 x < rb->max;
                 x++, index = (index - 1 + rb->max) % rb->max)
                printf("%d\t%lld\n", x, rb->data[index]);
            l4semaphore_up(&(win->sem));
            break;
        }
        case WINDOW_TYPE_FANCY_LIST:
        {
            int x, index;
            rt_mon_event_list_t * list;
            fancy_list_t * fl;

            list = (rt_mon_event_list_t *)win->data;

            fl = (fancy_list_t *)win->local;

            l4semaphore_down(&(win->sem));
            // clear local buffer
            printf("# index #\tvalue\taverage\n");
            for (x = 0, index = (fl->vals->index - 1 + fl->vals->max) %
                                 fl->vals->max;
                 x < fl->vals->max;
                 x++, index = (index - 1 + fl->vals->max) % fl->vals->max)
                printf("%d\t%lld\t%lld\n", x,
                       fl->vals->data[index], fl->avgs->data[index]);
            l4semaphore_up(&(win->sem));
            break;
        }
        }
        break;
    }
    case BUTTON_MINUS1:
    case BUTTON_MINUS2:
    case BUTTON_PLUS1:
    case BUTTON_PLUS2:
    {
        fancy_list_t *temp_fl;

        if (win->type != WINDOW_TYPE_FANCY_LIST)
            return;

        temp_fl = (fancy_list_t *)win->local;
        switch (function)
        {
        case BUTTON_MINUS1:
            temp_fl->high--;
            dope_cmdf(app_id, "entry_%d_a.set(-text \"%lld\")", fd,
                      temp_fl->high);
            break;
        case BUTTON_MINUS2:
            temp_fl->low--;
            dope_cmdf(app_id, "entry_%d_b.set(-text \"%lld\")", fd,
                      temp_fl->low);
            break;
        case BUTTON_PLUS1:
            temp_fl->high++;
            dope_cmdf(app_id, "entry_%d_a.set(-text \"%lld\")", fd,
                      temp_fl->high);
            break;
        case BUTTON_PLUS2:
            temp_fl->low++;
            dope_cmdf(app_id, "entry_%d_b.set(-text \"%lld\")", fd,
                      temp_fl->low);
            break;
        }

        break;
    }
    }
}
