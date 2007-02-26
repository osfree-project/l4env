/*
 * \brief   Petze library
 * \date    2003-07-31
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This library must be linked to  the program, Petze is about
 * to observe. It forwards the logged data to the Petze server
 * for storage and later analysis.
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <stdio.h>

/*** L4 INCLUDES ***/
#include <l4/petze/petze-client.h>
#include <l4/names/libnames.h>


l4_threadid_t petze_tid;
static CORBA_Object petze_server;
CORBA_Environment env = dice_default_environment;


/*** CONTACT THE PETZE SERVER ***
 *
 * When called the first time, request the petze server at names.
 */
static inline void contact_petze_server(void) {
	if (!petze_server) {
		if (names_waitfor_name("Petze", &petze_tid, 100) == 0) {
			printf("Petze is not registered at names!\n");
			return;
		}
		petze_server = &petze_tid;
	}
}


/*** TROJAN MALLOC FUNCTION ***
 *
 * This function is called instead the original malloc() function
 * from  instrumented code.  In addition to  the size argument it
 * provides a  pool identifier  which provides  information about
 * where  the malloc call  was performed: for example the name of
 * the source code file.
 */
void *petz_malloc(char *poolname, unsigned int size);
void *petz_malloc(char *poolname, unsigned int size) {
	void *addr = malloc(size);

	contact_petze_server();

	/* inform petze server about the called function */
	if (addr && petze_server) {
		petze_malloc_call(petze_server,poolname,(int)addr,size,&env);
	}
	
	return addr;
}

void *petz_free(char *poolname, void *addr);
void *petz_free(char *poolname, void *addr) {
	free(addr);
	
	contact_petze_server();

	/* inform petze server about the called function */
	if (addr && petze_server) {
		petze_free_call(petze_server,poolname,(int)addr,&env);
	}
	
	return addr;
}
