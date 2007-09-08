#include <l4/dde/ddekit/lock.h>
#include <l4/dde/ddekit/memory.h>

#include <l4/lock/lock.h>

struct ddekit_lock {
	l4lock_t lock;
};

void _ddekit_lock_init(struct ddekit_lock **mtx) {
	*mtx = (struct ddekit_lock *) ddekit_simple_malloc(sizeof(struct ddekit_lock));
	(*mtx)->lock = L4LOCK_UNLOCKED;
}

void _ddekit_lock_deinit(struct ddekit_lock **mtx) {
	ddekit_simple_free(*mtx);
	*mtx = NULL;
}

void _ddekit_lock_lock(struct ddekit_lock **mtx) {
	l4lock_lock(&(*mtx)->lock);
}

/* returns 0 on success, != 0 if it would block */
int _ddekit_lock_try_lock(struct ddekit_lock **mtx) {
	return l4lock_try_lock(&(*mtx)->lock) ? 0 : 1;
}

void _ddekit_lock_unlock(struct ddekit_lock **mtx) {
	l4lock_unlock(&(*mtx)->lock);
}

