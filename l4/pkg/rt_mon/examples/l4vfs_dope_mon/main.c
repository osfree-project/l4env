#include <l4/dm_phys/dm_phys.h>
#include <l4/l4rm/l4rm.h>
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/semaphore/semaphore.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/types.h>
#include <l4/thread/thread.h>

#include <l4/dope/dopelib.h>
#include <l4/dope/vscreen.h>
#include <l4/libvfb/vfb.h>
#include <l4/libvfb/icons.h>

#include <l4/rt_mon/defines.h>
#include <l4/rt_mon/l4vfs_monitor.h>
#include <l4/rt_mon/types.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "tree.h"
#include "main.h"
#include "wins.h"
#include "vis.h"


void create_vscreen_for_icon(char * name);


static l4thread_t ut;
long app_id;
tree_node_t * root;
int counter = 0;


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
        l4_sleep(100);
    }
}


static void close_window(wins_t *wins);
static void close_window(wins_t *wins)
{
    int ret;
    int fd = wins->fd;

    wins->visible = 0;
    l4semaphore_down(&(wins->sem));

    // release it
    ret = rt_mon_release_ds(fd);
    if (ret)
    {
        LOG("Something went wrong on releasing ds, ret = %d, ignored", ret);
    }

    vis_close(wins);

    l4semaphore_up(&(wins->sem));

}

/* Process events on tree nodes.  Handle expand, collapse, open and close.
 */
void press_callback(dope_event *e, void *arg)
{
    tree_node_t * node = (tree_node_t *)arg;
    tree_node_t * temp;
    int exp, ret;

    if (node->type == NODE_LEAF)
    {
        void * p;
        int fd, win_index, type;
        char * name;
        struct stat sb;

        name = tree_node_get_path(node);
        ret = stat(name, &sb);
        if (ret != 0)
        {
            LOG("Could not stat file, giving up!");
        }

        win_index = wins_index_for_id(sb.st_ino);
        if (win_index >= 0)
        {
            close_window(&(wins[win_index]));
            return;
        }
        else
            win_index = wins_get_new();

        if (win_index < 0)
        {
            LOG("Cannot open more windows!");
            return;
        }

        wins[win_index].id = sb.st_ino;
        LOG("Got ino = %d", wins[win_index].id);

        fd = rt_mon_request_ds(&(p), name);
        if (fd < 0)
        {
            LOG("rt_mon_request_ds failed on object, ret = %d", fd);
            return;
        }
        wins[win_index].fd = fd;
        free(name);

        type = ((rt_mon_data *)p)->type;
        LOG("found type %d", type);
        wins[win_index].data = p;
        switch (type)
        {
        case RT_MON_TYPE_HISTOGRAM:
            wins[win_index].type = WINDOW_TYPE_HISTO;
            break;
        case RT_MON_TYPE_HISTOGRAM2D:
            wins[win_index].type = WINDOW_TYPE_HISTO2D;
            break;
        case RT_MON_TYPE_EVENT_LIST:
        case RT_MON_TYPE_SHARED_LIST:
            wins[win_index].type = WINDOW_TYPE_FANCY_LIST;
            break;
        case RT_MON_TYPE_SCALAR:
            wins[win_index].type = WINDOW_TYPE_SCALAR;
            break;
        default:
            LOG("Unknown sensors type found, ignored!");
            wins_init_win(win_index);
            return;
        }
        vis_create(&(wins[win_index]));
        wins[win_index].visible = 1;
    }
    else
    {
        exp = tree_node_get_expanded(node);
        if (exp)
        {
            // remove all grand childs
            temp = node->first_child;
            while (temp)
            {
                tree_node_remove_childs(temp);
                temp = temp->next;
            }
        }
        else
        {
            char * name;
            // empty self
            tree_node_remove_childs(node);
            name = tree_node_get_path(node);
            tree_node_populate_from_dir(node, name, 2);
            free(name);
        }
        tree_node_set_expanded(node, ! exp);
        tree_node_create_dope_repr(root);
        dope_cmdf(app_id, "buffer_grid.place(tree_container_%d,"
                  " -column 0 -row 0)", root->id);
    }
}

void create_vscreen_for_icon(char * name)
{
    int w, h, r, g, b, a, x, y;
    l4_int16_t * vscr;
    const vfb_icon_t * icon;

    dope_cmdf(app_id, "%s = new VScreen()", name);
    icon = vfb_icon_get_for_name(name);
    w = icon->width;
    h = icon->height;
    dope_cmdf(app_id, "%s.setmode(%d, %d, \"RGB16\")", name, w, h);
    vscr = vscr_get_fb(app_id, name);
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
        {
            vfb_icon_get_pixel(icon, x, y, &r, &g, &b, &a);
            if (a == 0)
            {
                r = 112;
                g = 128;
                b = 144;
            }
            vfb_setpixel_rgb(vscr, w, h, x, y, r, g, b);
        }
}

int main(int argc, char* argv[])
{
    int ret;

    while ((ret = dope_init()))
    {
        printf("Waiting for DOpE ...\n");
        l4_sleep(100);
    }

    app_id = dope_init_app("DOpE RtMon");

    create_vscreen_for_icon("tree_plus");
    create_vscreen_for_icon("tree_minus");
    create_vscreen_for_icon("tree_corner");
    create_vscreen_for_icon("tree_cross");
    create_vscreen_for_icon("tree_vert");
    create_vscreen_for_icon("tree_none");
    create_vscreen_for_icon("folder_grey");
    create_vscreen_for_icon("misc");
    create_vscreen_for_icon("unknown");

    wins_init();

    // setup some nodes from the l4vfs namespace
    root = tree_node_create("/", counter++, NODE_DIR);
    tree_node_populate_from_dir(root, "/", 1);

    tree_node_create_dope_repr(root);

    // window
    dope_cmd(app_id, "window = new Window()");
    dope_cmd(app_id, "window.open()");
    dope_cmd(app_id, "buffer_grid = new Grid()");
    dope_cmd(app_id, "buffer_grid.rowconfig(0)");
    dope_cmd(app_id, "buffer_grid.rowconfig(1)");
    dope_cmd(app_id, "buffer_grid.columnconfig(0)");
    dope_cmd(app_id, "buffer_grid.columnconfig(1)");
    dope_cmd(app_id, "main_frame = new Frame(-scrollx yes -scrolly yes"
                     " -content buffer_grid)");
    dope_cmdf(app_id, "buffer_grid.place(tree_container_%d, -column 0 -row 0)",
              root->id);
    dope_cmd(app_id, "window.set(-content main_frame)");

    ut = l4thread_create(update_thread, NULL, L4THREAD_CREATE_ASYNC);

    dope_eventloop(app_id);

    return 0;
}
