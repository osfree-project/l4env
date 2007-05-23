/*
 * \brief   Petze library
 * \date    2003-07-31
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This library must be linked to the program, Petze is about
 * to observe. It forwards the logged data to the Petze server
 * for storage and later analysis.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>

/*** L4 INCLUDES ***/
#include <l4/petze/petze-client.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>

#include <l4/petze/petze.h>

/*** LOCAL ***/
#include "local.h"


static int threadlib_key;
static l4_threadid_t petze_tid;
static CORBA_Object petze_server;

static int retry = 5;

/*** CONTACT THE PETZE SERVER ***
 *
 * When called the first time, request the petze server at names.
 */
static inline void contact_petze_server(void) {
	if (!petze_server && retry) {
		if (names_waitfor_name("Petze", &petze_tid, 500) == 0) {
			if (--retry)
				printf("Petze is not registered at names - retrying.\n");
			else
				printf("Petze is not registered at names - giving up.\n");
			return;
		}
		petze_server = &petze_tid;
	}
}

/*** PREVENT DOUBLE ACCOUNTING ***
 *
 * We indicate intention to account via enter_accounting() and account
 * completion via leave_accounting(). The actual check if we have to account
 * is done via must_account().
 */
static void enter_accounting(void) {
	int level;

	if (!threadlib_key) {
		threadlib_key = l4thread_data_allocate_key();
		if (!threadlib_key)
			printf("Petze got no threadlib key!\n");
	}
	level = (int)l4thread_data_get_current(threadlib_key);
	l4thread_data_set_current(threadlib_key, (void *)(level + 1));
}

static void leave_accounting(void) {
	int level;

	level = (int)l4thread_data_get_current(threadlib_key);
	l4thread_data_set_current(threadlib_key, (void *)(level - 1));
}

static int must_account(void) {
	return ((int)(l4thread_data_get_current(threadlib_key)) == 1);
}


/*** TROJAN MALLOC FUNCTION ***
 *
 * This function is called instead the original malloc() function
 * from  instrumented code.  In addition to  the size argument it
 * provides a  pool identifier  which provides  information about
 * where  the malloc call  was performed: for example the name of
 * the source code file.
 */
void *petz_malloc(char *poolname, unsigned int size) {
	void *addr;
	CORBA_Environment env = dice_default_environment;

	enter_accounting();

	addr = __real_malloc(size);
//	memset(addr, 0, size);

	contact_petze_server();

	/* inform petze server about the called function */
	if (addr && petze_server && must_account()) {
		petze_malloc_call(petze_server, poolname, (int)addr, size, &env);
	}

	leave_accounting();
	return addr;
}


void *petz_calloc(char *poolname, unsigned int nmemb, unsigned int size) {
	void *addr;
	CORBA_Environment env = dice_default_environment;

	enter_accounting();

	addr = __real_calloc(nmemb, size);

	contact_petze_server();

	/* inform petze server about the called function */
	if (addr && petze_server && must_account()) {
		petze_malloc_call(petze_server, poolname, (int)addr, (nmemb*size), &env);
	}

	leave_accounting();
	return addr;
}


/*** INFORM PETZE SERVER TO FREE AND MALLOC NEW SIZE ***/
void *petz_realloc(char *poolname, void *addr, unsigned int size) {
	void *newaddr;
	CORBA_Environment env = dice_default_environment;

	enter_accounting();

	contact_petze_server();

	/* inform petze server about the called function */
	if (addr && petze_server && must_account()) {
		petze_free_call(petze_server, poolname, (int)addr, &env);
	}

	newaddr = __real_realloc(addr, size);

	/* inform petze server about the called function */
	if (newaddr && petze_server && must_account()) {
		CORBA_exception_free(&env);
		petze_malloc_call(petze_server, poolname, (int)newaddr, size, &env);
	}

	leave_accounting();
	return newaddr;
}


void petz_free(char *poolname, void *addr) {
	CORBA_Environment env = dice_default_environment;

	enter_accounting();

	__real_free(addr);

	contact_petze_server();

	/* inform petze server about the called function */
	if (addr && petze_server && must_account()) {
		petze_free_call(petze_server, poolname, (int)addr, &env);
	}

	leave_accounting();
}


void *petz_cxx_new(char *poolname, unsigned int size) {
	void *addr;
	CORBA_Environment env = dice_default_environment;

	enter_accounting();

	addr = REAL_CXX_NEW(size);
	contact_petze_server();

	/* inform petze server about the called function */
	if (addr && petze_server && must_account()) {
		petze_malloc_call(petze_server, poolname, (int)addr, size, &env);
	}

	leave_accounting();
	return addr;
}


void petz_cxx_free(char *poolname, void *addr) {
	CORBA_Environment env = dice_default_environment;

	enter_accounting();

	REAL_CXX_FREE(addr);

	contact_petze_server();

	/* inform petze server about the called function */
	if (addr && petze_server && must_account()) {
		petze_free_call(petze_server, poolname, (int)addr, &env);
	}

	leave_accounting();
}
