/*
 * \brief   DOpE Petz analysing tool
 * \date    2002-11-21
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>

/*** L4 INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/petze/petze-client.h>
#include <l4/names/libnames.h>
#include <l4/dope/dopelib.h>

char LOG_tag[9] = "OllePetze";
l4_ssize_t l4libc_heapsize = 500*1024;

static long app_id;
static l4_threadid_t petze_tid;
static CORBA_Object petze_server = &petze_tid;
static CORBA_Environment env = dice_default_environment;


static void dump_callback(dope_event *e,void *arg) {
	petze_dump_call(petze_server,&env);
}

static void reset_callback(dope_event *e,void *arg) {
	petze_reset_call(petze_server,&env);
}


int main(int argc,char **argv) {

	while (names_waitfor_name("Petze", petze_server, 2000) == 0) {
		printf("Petze is not registered at names!\n");
	}

	dope_init();
	app_id = dope_init_app("Olle Petze");

	dope_cmd(app_id,"a=new Window()");
	dope_cmd(app_id,"g=new Grid()");
	
	dope_cmd(app_id,"dump=new Button()");
	dope_cmd(app_id,"dump.set(-text \"Dump Statistics\")");
	dope_cmd(app_id,"reset=new Button()");
	dope_cmd(app_id,"reset.set(-text \"Reset Statistics\")");
	dope_cmd(app_id,"a.set(-content g)");
	dope_cmd(app_id,"g.place(dump, -column 1 -row 1)");
	dope_cmd(app_id,"g.place(reset, -column 1 -row 2)");
	dope_cmd(app_id,"a.open()");
	
	dope_bind(app_id,"dump",  "click", dump_callback,  (void *)0);
	dope_bind(app_id,"reset", "clack", reset_callback, (void *)0);
	
	dope_eventloop(app_id);
	return 0;
}
