/*!
 * \file   l4/pkg/loader/examples/launchpad/main.c
 * \brief  Launch applications with loader by clicking buttons.
 *
 * \date   10/04/2004
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>
#include <l4/names/libnames.h>
#include <l4/loader/loader-client.h>
#include <l4/loader/loader.h>
#include <l4/util/getopt.h>
#include <l4/util/macros.h>
#include <l4/dope/dopelib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char LOG_tag[9] = "launchpad";
l4_ssize_t l4libc_heapsize = 500*1024;

static struct option long_opts[] =
{
  { "app",    1, 0, 'a' },
  { "name",   1, 0, 'n' },
  { "single", 0, 0, 's' },
  { "fprov",  1, 0, 'f' },
  { 0,        0, 0, 0   }
};

l4_threadid_t loader_id = L4_INVALID_ID;
l4_threadid_t fprov_id  = L4_INVALID_ID;

const char *fprov_name = "TFTP";

enum { APP_LIST_SIZE = 10 };
struct app_struct {
  char *app;           /* Path of the app */
  char *name;          /* Optional better name may be given */
  char single;         /* Only started once or multiple times? */
  char running;        /* if single, is it running? */
};
struct app_struct app_list[APP_LIST_SIZE];
int app_list_idx;

long app_id;

static int init_me(void)
{
  if (!names_waitfor_name("LOADER", &loader_id, 30000))
    {
      printf("Dynamic loader LOADER not found\n");
      return 1;
    }

  if (!names_waitfor_name(fprov_name, &fprov_id, 30000))
    {
      printf("File provider \"%s\" not found\n", fprov_name);
      return 1;
    }

  printf("File provider is \"%s\"\n", fprov_name);

  return 0;
}

static void load_app(const char *app)
{
  int error;
  CORBA_Environment env = dice_default_environment;
  l4dm_dataspace_t dummy_ds = L4DM_INVALID_DATASPACE;
  char error_msg[1024] = "";
  char *ptr = error_msg;
  l4_taskid_t task_ids[l4loader_MAX_TASK_ID];

  if ((error = l4loader_app_open_call(&loader_id, &dummy_ds, app,
                                      &fprov_id, 0, task_ids,
                                      &ptr, &env)))
    {
      printf("Error %d (%s) loading application\n", error, l4env_errstr(error));
      if (*error_msg)
        printf("(Loader said: '%s')\n", error_msg);
    }

}

static void callback_press(dope_event *e, void *arg)
{
  int nr = (int)arg;

  if (nr >= app_list_idx)
    return;

  if (!app_list[nr].single
      || (app_list[nr].single && !app_list[nr].running))
    {
      printf("loading app: %s\n", app_list[nr].app);
      load_app(app_list[nr].app);
    }
  if (app_list[nr].single && !app_list[nr].running)
    {
      dope_cmdf(app_id, "b%d.set(-state 1)", nr);
      app_list[nr].running = 1;
    }
}

static void parse_opts(int argc, char **argv)
{
  int c;

  while (1)
    {
      c = getopt_long(argc, argv, "a:f:n:s", long_opts, NULL);

      if (c == -1)
        break;

      switch (c)
        {
        case 'a':
          app_list[app_list_idx++].app = optarg;
          break;

	case 'n':
	  if (app_list_idx)
	    app_list[app_list_idx - 1].name = optarg;
	  else
	    printf("-n option needs -a before!\n");
	  break;

	case 's':
	  if (app_list_idx)
	    app_list[app_list_idx - 1].single = 1;
	  else
	    printf("-s option needs -a before!\n");
	  break;

        case 'f':
          fprov_name = optarg;
          break;
       };
    }
}

static char *simple_basename(char *name)
{
  char *p;

  if ((p = strrchr(name, '/')))
    return p + 1;
  return name;
}

static char *get_app_name(int nr)
{
  if (nr < app_list_idx)
    {
      if (app_list[nr].name)
	return app_list[nr].name;
      return simple_basename(app_list[nr].app);
    }
  return NULL;
}

int main(int argc, char **argv)
{
  int i;

  parse_opts(argc, argv);

  if (!app_list_idx)
    {
      printf("Usage: %s -a app1 -a app2 -a ... -f fprov\n", *argv);
      return 1;
    }

  if (init_me())
    return 1;

  dope_init();

  app_id = dope_init_app("LaunchPad");

  dope_cmd(app_id, "mainwin = new Window()");
  dope_cmd(app_id, "grid = new Grid()");
  dope_cmd(app_id, "mainwin.set(-content grid)");
  dope_cmd(app_id, "mainwin.set(-w 130 -h 1)");

  for (i = 0; i < app_list_idx; i++)
    {
      char buf[5] = { 0, };

      dope_cmdf(app_id, "b%d = new Button()", i);
      dope_cmdf(app_id, "b%d.set(-text \"%s\")", i, get_app_name(i));
      dope_cmdf(app_id, "grid.place(b%d, -column 1 -row %d)", i, i);
      snprintf(buf, sizeof(buf), "b%d", i);
      dope_bind(app_id, buf, app_list[i].single ? "press" : "commit",
                callback_press, (void *)i);
    }

  dope_cmd(app_id, "mainwin.open()");

  dope_eventloop(app_id);

  return 0;
}
