#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "sync.h"

struct dopelib_mutex {
	pthread_mutex_t m;
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
	if (init_state) dopelib_lock_mutex(new_mutex);
	return new_mutex;
}


/** FREE MUTEX
 */
void dopelib_destroy_mutex(struct dopelib_mutex *m) {
	free(m);
}


/** LOCK MUTEX
 *
 * \param mutex pointer to mutex which should be locked
 */
void dopelib_lock_mutex(struct dopelib_mutex *m) {
	pthread_mutex_lock(&m->m);
}


/** UNLOCK MUTEX
 *
 * \param mutex pointer to mutex which should be unlocked
 */
void dopelib_unlock_mutex(struct dopelib_mutex *m) {
	pthread_mutex_unlock(&m->m);
}

