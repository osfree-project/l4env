#include <l4/names/libnames.h>
#include "test-server.h"
#include <stdio.h>
#include <stdlib.h>

l4_ssize_t l4libc_heapsize = 500*1024;
char LOG_tag[9] = "dcetst";

CORBA_long test_event_component(CORBA_Object _dice_corba_obj,
                                            const dope_event *e,
                                            const_CORBA_char_ptr bindarg,
                                            CORBA_Environment *_dice_corba_env) {
	printf("Server(event_component): got event (type=%lu, bindarg=%s)\n",e->_d,bindarg);
	switch(e->_d) {
	case 1:
		printf("Server(event_component): COMMAND event (\"%s\")",e->_u.command.cmd);
		break;
	case 2:
		printf("Server(event_component): MOTION event (rx=%d, ry=%d, ax=%d, ay=%d)\n",
			(int)e->_u.motion.rel_x, (int)e->_u.motion.rel_y, 
			(int)e->_u.motion.abs_x, (int)e->_u.motion.abs_y);
		break;
	case 3:
		printf("Server(event_component): PRESS event (code=%lu)\n",e->_u.press.code);
		break;
	case 4:
		printf("Server(event_component): RELEASE event (code=%lu)\n",e->_u.release.code);
		break;
	}
	printf("Server(event_component): finished.\n");
	return 1234;
}


void *CORBA_alloc(unsigned long size) {
	return malloc(size);
}


int main(int argc, char **argv) {
	if (!names_register((const char*)LOG_tag)) {
		printf("Server(main): can't register at names\n");
		return -1;
	}
	printf("Server(main): registered name... entering server loop\n");
	test_server_loop(NULL);
	return 0;
}


