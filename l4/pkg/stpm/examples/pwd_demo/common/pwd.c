/*
 * \brief   Simple password input program
 * \date    2004-03-11
 * \author  Bernhard Kauer <kauer@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2004 Bernhard Kauer <kauer@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#include <stdio.h>
#include <l4/dope/dopelib.h>

// for kill
#include <l4/env/env.h>
#include <l4/env/errno.h>
#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include <l4/loader/loader-client.h>


int l4libc_heapsize = 64*1024;
int app_id;


struct pwddata{
  int len;
  char *pwdwidget;
  char *statuswidget;
  char pwd[9];  
};


#define DEBUG(fmt, ...) printf("%s()" fmt "\n",__func__  , ##__VA_ARGS__ )

/**
 * A function to kill us at the loader.
 */
int kill_myself(void){
  l4_threadid_t loader_id;
  l4_taskid_t own_id;

  CORBA_Environment env = dice_default_environment;
  int error;

  if (!names_waitfor_name("LOADER", &loader_id, 3000))
    {
      printf("LOADER not found");
      return -1;
    }


  own_id = l4thread_l4_id(l4thread_myself());
  if ((error = l4loader_app_kill_call(&loader_id, &own_id, 0, &env)))
    {
      printf("Error %d (%s) killing application\n", 
	     error, l4env_errstr(error));
      return error;
    }
  
  printf("  successfully killed.\n");
  return 0;
}

static int abort_loop;

void mydope_eventloop(long id) {
	dope_event *e;
	char  *bindarg;
	while (!abort_loop) {
		dopelib_wait_event(id, &e, &bindarg);

		/* test if cookie is valid */
		if ((bindarg[0]=='#') && (bindarg[1]=='!')) {
		
			/* determine callback adress and callback argument */
			unsigned long num1 = 0;
			unsigned long num2 = 0;
			char  *s = bindarg + 3;
			
			while (*s == ' ') s++;
			while ((*s != 0) && (*s != ' ')) {
				num1 = (num1<<4) + (*s & 15);
				if (*(s++)>'9') num1+=9;
			}
			if (*(s++) == 0) return;
			while ((*s != 0) && (*s != ' ')) {
				num2 = (num2<<4) + (*s&15);
				if (*(s++)>'9') num2+=9;
			}
			
			/* call callback routine */
			if (num1) ((void (*)(dope_event *,void *))num1)(e,(void *)num2);
		}
	}
}

static void quit_callback(dope_event *e,void *arg) {
  abort_loop=1;
  DEBUG("");
}


static void clear_callback(dope_event *e,void *arg) {  
  struct pwddata *pwd=(struct pwddata *) arg;
  if (arg){
    dope_cmdf(app_id, "%s.set(-text \"\")",pwd->pwdwidget);
    dope_cmdf(app_id, "%s.set(-text \"\")",pwd->statuswidget);
    pwd->len=0;
    pwd->pwd[0]=0;
  }
  else
    DEBUG("no arg");
}

static void input_callback(dope_event *e,void *arg) {
  struct pwddata *pwd=(struct pwddata *) arg;
  switch (e->type){
  case EVENT_TYPE_PRESS:
    {
      int ch;
      
      ch=dope_get_ascii(app_id,e->press.code);

      if (ch==8){
	pwd->len--;
	ch=0;
      } else if (ch==10){
#if BAD_ONE
	  dope_cmdf(app_id, "%s.set(-text \"Your password was '%s'!\")",pwd->statuswidget,pwd->pwd);
#else
	  dope_cmdf(app_id, "%s.set(-text \"I keep your password privat.\")",pwd->statuswidget,pwd->pwd);
#endif
	pwd->len=0;
	ch=0;
      } else if (ch<32){
	ch=0;
      }

      if (pwd->len<0) {
	ch = -1;
	pwd->len = 0;
      }
      else if (pwd->len>=sizeof(pwd->pwd)-1){
	ch = -1;
      }
      
      if (ch>=0) {
	pwd->pwd[pwd->len]=ch;
	if (ch!=0)
	  pwd->pwd[++pwd->len]=0;
	{
	  char buf[9];
	  strncpy(buf,"********",8);
	  buf[pwd->len]=0;
	  dope_cmdf(app_id, "%s.set(-text \"%s\")",pwd->pwdwidget,buf);
	}
      }
    }
    break;

  default:
    // ignore
    DEBUG("event %d ignored",e->type);
    break;
  }

}


int myapp_init(char *app_name,struct pwddata *pwd){

  if (dope_init())
    return -1;
  //if (!app_name)
  app_name="pwd";
  app_id = dope_init_app(app_name);
  
  if (app_id<0)
    return app_id;

  /* create menu window with one button for each effect */
  dope_cmd(app_id,"mainwin = new Window()");
  dope_cmd(app_id,"mainwin.set(-w 250 -h 150)");
  dope_cmdf(app_id,"mainwin.set(-title \"%s\")",app_name);

  dope_cmd(app_id,"mg = new Grid()");
  dope_cmd(app_id,"mainwin.set(-content mg)");

  dope_cmd(app_id,"og = new Grid()");
  dope_cmd(app_id, "mg.place(og,-column 1 -row 1)");
  
  dope_cmd(app_id, "bq = new Button()");
  dope_cmd(app_id, "bq.set(-text \"quit\")");
  dope_bind(app_id, "bq","press", quit_callback,NULL);
  dope_cmd(app_id, "og.place(bq,-column 1 -row 1 -padx 2 -pady 2)");

  dope_cmd(app_id, "bc = new Button()");
  dope_cmd(app_id, "bc.set(-text \"clear\")");
  dope_bind(app_id, "bc","press", clear_callback,pwd);
  dope_cmd(app_id, "og.place(bc,-column 2 -row 1 -padx 2 -pady 2)");


  dope_cmd(app_id, "l = new Label()");
  dope_cmd(app_id, "l.set(-text \"Password:\" -font title)");
  dope_cmd(app_id, "og.place(l,-column 1 -row 2 -padx 2 -pady 2)");

  dope_cmdf(app_id, "%s = new Button()",pwd->pwdwidget);
  dope_cmdf(app_id, "%s.set(-text \"\" -state 1)",pwd->pwdwidget);
  dope_cmdf(app_id, "og.place(%s,-column 2 -row 2 -padx 2 -pady 2)",pwd->pwdwidget);
  dope_bind(app_id, pwd->pwdwidget,"press", input_callback,pwd);

  dope_cmdf(app_id, "%s = new Label()",pwd->statuswidget);
  dope_cmdf(app_id, "%s.set(-text \"\" -font monospaced)",pwd->statuswidget);
  dope_cmdf(app_id, "mg.place(%s,-column 1 -row 2 -padx 2)",pwd->statuswidget);

  dope_cmd(app_id,"mainwin.open()");

  return 0;
}

void myapp_deinit(void){
  dope_cmd(app_id,"mainwin.close()");
  dope_deinit_app(app_id);
  dope_deinit();
}


int main(int argc, char **argv) {
  struct pwddata pwd;

  pwd.len=0;
  pwd.pwd[0]=0;
  pwd.pwdwidget="bi";
  pwd.statuswidget="lst";

  if (!myapp_init(argv[0],&pwd)){
    /* enter mainloop */
    mydope_eventloop(app_id);
    myapp_deinit();
  }
  return kill_myself();
}
