/**
 * \file   rt_mon/examples/dope_mon/main.c
 * \brief  Example monitoring server for DOpE.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 * \author Jork Loeser     <jork.loeser@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/dm_phys/dm_phys.h>
#include <l4/l4rm/l4rm.h>
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/util/rdtsc.h>
#include <l4/util/parse_cmd.h>
#include <l4/semaphore/semaphore.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/types.h>
#include <l4/thread/thread.h>

#include <l4/dope/dopelib.h>
#include <l4/dope/vscreen.h>

#include <l4/rt_mon/defines.h>
#include <l4/rt_mon/monitor.h>
#include <l4/rt_mon/types.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "wins.h"
#include "vis.h"

#define stringer( x ) xstringer( x )
#define xstringer( x ) # x

struct
{
    int leftalign;		/* align pool names to the left */
    int showload;		/* show CPU load in window title */
} conf;

long app_id;
static l4thread_t ut;
static int fd_counter = 0;

int _get_next_fd(void);
int _get_next_fd(void)
{
    return fd_counter++;
}

void dump_wins(wins_t wins, int i);

static void update_thread(void *arg)
{
    int i;

    while (1)
    {
        for (i = 0; i < MAX_WINS; i++)
        {
            if (wins[i].visible)
            {
                l4semaphore_down(&wins[i].sem);
                vis_refresh(&(wins[i]));
                l4semaphore_up(&wins[i].sem);
            }
        }
        l4_sleep(500);
    }
}

static void close_window(wins_t *wins);
static void close_window(wins_t *wins)
{
    int ret;

    wins->visible = 0;
    l4semaphore_down(&(wins->sem));

    // release it
    ret = l4rm_detach(wins->data);
    if (ret)
    {
        LOG("Could not detach, ignored!");
    }
    ret = rt_mon_release_ds(wins->id);
    if (ret)
    {
        LOG("Could not release dataspace, ignored!");
    }

    vis_close(wins);

    l4semaphore_up(&(wins->sem));
}

static int dss_name_comp(const void *n1, const void *n2)
{
    return strcmp(((rt_mon_dss *)n1)->name,
		  ((rt_mon_dss *)n2)->name);
}

static void press_callback(dope_event *e, void *arg)
{
    int id = (int)arg;

    switch (id)
    {
    case 0:  // refresh available dataspaces
    case 1:  // refresh available dataspaces, sort alphabetically
    {
        int i, ret;
        rt_mon_dss dss[MAX_WINS];
        char cmd[200];
        rt_mon_dss *dss_p;
        dss_p = dss;

        LOG("Refresh");

        // close the old windows
        for (i = 0; i < MAX_WINS; i++)
        {
            if (wins[i].visible)
            {
                close_window(&(wins[i]));
            }
        }

        ret = rt_mon_list_ds(&dss_p, MAX_WINS);
	if(id == 1)
	{
	    qsort(dss, ret, sizeof(rt_mon_dss), dss_name_comp);
	}
        dope_cmd(app_id, "g1.remove(g_cont)");
        dope_cmd(app_id, "g_cont = new Grid()");
        dope_cmd(app_id, "g1.place(g_cont, -column 1 -row 2)");

        for (i = 0; i < ret; i++)
        {
            dope_cmd(app_id, "temp = new Label()");

            snprintf(cmd, 200, conf.leftalign
		     ? "temp.set(-text \"%2d: %-"
		       stringer( RT_MON_NAME_LENGTH ) "s\")"
		     : "temp.set(-text \"%2d: %"
		       stringer( RT_MON_NAME_LENGTH ) "s\")",
                     dss[i].id, dss[i].name);
            dope_cmd(app_id, cmd);
            dope_cmdf(app_id, "g_cont.place(temp, -column 1 -row %d -align w)",
                      i + 1);
            dope_cmdf(app_id, "s_b%d = new Button()", dss[i].id);
            dope_cmdf(app_id, "s_b%d.set(-text Show)", dss[i].id);
            snprintf(cmd, 200, "g_cont.place(s_b%d, -column 2 -row %d)",
                     dss[i].id, i + 1);
            dope_cmd(app_id, cmd);
            snprintf(cmd, 200, "s_b%d", dss[i].id);
            dope_bind(app_id, cmd, "clack", press_callback,
                      (void *)(BIND_BUTTON_OFFSET + dss[i].id));
            LOG("bound on  = %d", BIND_BUTTON_OFFSET + dss[i].id);
        }
        break;
    }
    case 2:  // exit
    {
        int i;
        LOG("Exit");
        for (i = 0; i < MAX_WINS; i++)
            if (wins[i].visible)
                dope_cmdf(app_id, "win_%d.close()", i);
        dope_cmd(app_id, "win.close()");
        l4thread_shutdown(ut);
        exit(0);
        break;
    }
    default:                           // probably a show button
    {
        int index, ret, type;
        l4dm_dataspace_t ds;
        l4_size_t size;

        if (id < BIND_BUTTON_OFFSET)   // unknown case
            break;

        id = id - BIND_BUTTON_OFFSET;  // convert to original id
        index = wins_index_for_id(id); // get local table index
        if (index < 0)                 // nothing found
            index = wins_get_new();    // so allocate a new one
        if (index < 0)                 // still no good?
            break;                     // no entry left, should not happen

        if (wins[index].visible)       // hide window
        {
            close_window(&(wins[index]));
            dope_cmdf(app_id, "s_b%d.set(-text Show)", id);
        }
        else                           // open a new window etc.
        {
            LOG("Opening %d ...", index);
            dope_cmdf(app_id, "s_b%d.set(-text Hide)", id);

            // request dataspace
            ret = rt_mon_request_ds(id, &ds);
            if (ret)
            {
                LOG("Could not get dataspace access!");
                break;
            }
            // map it
            ret = l4dm_mem_size(&ds, &size);
            if (ret)
            {
                LOG("Could not get size for dataspace!");
                break;
            }

            ret = l4rm_attach(&ds, size, 0, L4DM_RW, &(wins[index].data));
            if (ret)
            {
                LOG("Could not attach dataspace!");
                break;
            }

            // display data
            type = ((rt_mon_data *)wins[index].data)->type;
            wins[index].id   = id;
            wins[index].fd   = _get_next_fd();
            LOG("Type = %d", type);
            switch (type)
            {
            case RT_MON_TYPE_HISTOGRAM:
                wins[index].type = WINDOW_TYPE_HISTO;
                break;
            case RT_MON_TYPE_HISTOGRAM2D:
                wins[index].type = WINDOW_TYPE_HISTO2D;
                break;
            case RT_MON_TYPE_EVENT_LIST:
            case RT_MON_TYPE_SHARED_LIST:
                wins[index].type = WINDOW_TYPE_FANCY_LIST;
                break;
            case RT_MON_TYPE_SCALAR:
                wins[index].type = WINDOW_TYPE_SCALAR;
                break;
            default:
                LOG("Unknown sensors type found, ignored!");
                wins_init_win(index);
                return;
            }
            vis_create(&(wins[index]));
            wins[index].visible = 1;
        }
    }
    }
}

void dump_wins(wins_t wins, int i)
{
    LOG("wins[%d]", i);
    LOG("visible = %d", wins.visible);
    LOG("type    = %d", wins.type);
    LOG("id      = %d", wins.id);
    LOG("data    = %p", wins.data);
    LOG("vscr    = %p", wins.vscr);
    LOG("w, h    = %d, %d", wins.w, wins.h);
}


static void load_update(void *arg)
{
    l4_uint32_t tsc, pmc, new_tsc, new_pmc;

    /* read tsc and performance measurement counter with low overhead */
    tsc = l4_rdtsc_32();
    /* "-loadcnt" to fiasco counts unused (halt) cycles in pmc 0 */
    pmc = l4_rdpmc_32(0);

    while(1)
    {
	l4_uint32_t load;

	l4_sleep(1000);
	new_tsc = l4_rdtsc_32();
	new_pmc = l4_rdpmc_32(0);
	load = (new_pmc - pmc) / ((new_tsc - tsc) / 1000);
	pmc = new_pmc;
	tsc = new_tsc;

	dope_cmdf(app_id, "win.set(-title \"RtMon: %d.%01d%% CPU\")",
		  load / 10, load % 10);
    }
}


int main(int argc, const char* argv[])
{
    int ret;

    if(parse_cmdline(&argc, &argv,
		     'a', "leftalign", "align names to the left",
		     PARSE_CMD_SWITCH, 1, &conf.leftalign,
		     'l', "load", "show CPU load (-loadcnt fiasco arg)",
		     PARSE_CMD_SWITCH, 1, &conf.showload,
		     0))
        return 1;

    while ((ret = dope_init()))
    {
        printf("Waiting for DOpE ...\n");
        l4_sleep(100);
    }
    wins_init();

    // fixme: check ret
    app_id = dope_init_app("DOpE RtMon");

    // build main window
    #include "main_window.i"

    if (conf.showload)
    {
	l4thread_create_named(load_update, ".showload", 0,
			      L4THREAD_CREATE_ASYNC);
    }
    // a small resizable window to interactively generate specificly
    // sized dope repaints
//    dope_cmd(app_id, "temp = new Window()");
//    dope_cmd(app_id, "temp.open()");

    dope_bind(app_id, "b_ref", "clack", press_callback, (void *)0);
    dope_bind(app_id, "b_sort", "clack", press_callback, (void *)1);
    dope_bind(app_id, "b_exit", "clack", press_callback, (void *)2);

    ut = l4thread_create(update_thread, NULL, L4THREAD_CREATE_ASYNC);

    dope_eventloop(app_id);

    return 0;
}
