#include <l4/semaphore/semaphore.h>
#include <stdlib.h>
#include <string.h>
#include "sync.h"

struct dopelib_mutex {
	l4semaphore_t sem;
};


/** CREATE NEW MUTEX
 *
 * \param init_state not implemented yet
 * \return pointer to newly created mutex
 */
struct dopelib_mutex *dopelib_create_mutex(int init_state) {
	struct dopelib_mutex *new_mutex = malloc(sizeof(struct dopelib_mutex));
	if (!new_mutex) return NULL;
	memset(new_mutex, 0, sizeof(struct dopelib_mutex));
	
	if (init_state) {
		new_mutex->sem = L4SEMAPHORE_LOCKED;
	} else {
		new_mutex->sem = L4SEMAPHORE_UNLOCKED;
	};
	return new_mutex;
}


/** FREE MUTEX
 */
void dopelib_destroy_mutex(struct dopelib_mutex *m) {
	if (!m) return;
	free(m);
}


/** LOCK MUTEX
 *
 * \param mutex pointer to mutex which should be locked
 */
void dopelib_lock_mutex(struct dopelib_mutex *m) {
	if (!m) return;
	l4semaphore_down(&m->sem);
}


/** UNLOCK MUTEX
 *
 * \param mutex pointer to mutex which should be unlocked
 */
void dopelib_unlock_mutex(struct dopelib_mutex *m) {
	if (!m) return;
	l4semaphore_up(&m->sem);
}

