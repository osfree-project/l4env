/*
 * \brief   DOpE Petz analysing tool
 * \date    2002-11-21
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>

/*** L4 INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/petze/petzetrigger.h>
#include <l4/dope/dopelib.h>

l4_ssize_t l4libc_heapsize = 512*1024;
char LOG_tag[9] = "OllePetze";

static long app_id;

static void dump_callback(dope_event *e,void *arg) {
	printf("--- Olle Petze dump request ---\n");
	petze_dump();
}


static void reset_callback(dope_event *e,void *arg) {
	printf("--- Olle Petze reset ---\n");
	petze_reset();
}


int main(int argc,char **argv) {

	if (dope_init()) return -1;

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
