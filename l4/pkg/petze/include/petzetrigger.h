#ifndef _L4_PETZE_PETZETRIGGER_H_
#define _L4_PETZE_PETZETRIGGER_H_

#include <l4/petze/petze-client.h>
#include <l4/names/libnames.h>

#define PETZE_SERVER_NAME "Petze"

/*** MAKE PETZE DUMP ALL COLLECTED INFORMATION ***/
static inline void petze_dump(void) {
	l4_threadid_t petze_tid;
	CORBA_Environment env = dice_default_environment;
	if (names_query_name(PETZE_SERVER_NAME, &petze_tid))
		petze_dump_call(&petze_tid, &env);
}


/*** MAKE PETZE FORGET ALL COLLECTED INFORMATION ***/
static inline void petze_reset(void) {
	l4_threadid_t petze_tid;
	CORBA_Environment env = dice_default_environment;
	if (names_query_name(PETZE_SERVER_NAME, &petze_tid))
		petze_reset_call(&petze_tid, &env);
}

#endif /* _L4_PETZE_PETZETRIGGER_H_ */
