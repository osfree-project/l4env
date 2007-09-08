#ifndef _ddekit_lock_h
#define _ddekit_lock_h

struct ddekit_lock;

/** Initialize a DDEKit lock. 
 *
 * \ingroup DDEKit_synchronization
 */
void _ddekit_lock_init    (struct ddekit_lock **mtx);

/** Uninitialize a DDEKit lock.
 *
 * \ingroup DDEKit_synchronization
 */
void _ddekit_lock_deinit  (struct ddekit_lock **mtx);

/** Acquire a lock.
 *
 * \ingroup DDEKit_synchronization
 */
void _ddekit_lock_lock    (struct ddekit_lock **mtx);

/** Acquire a lock, non-blocking.
 *
 * \ingroup DDEKit_synchronization
 */
int  _ddekit_lock_try_lock(struct ddekit_lock **mtx);

/** Unlock function.
 *
 * \ingroup DDEKit_synchronization
 */
void _ddekit_lock_unlock  (struct ddekit_lock **mtx);

// definition of ddekit_lock_t
typedef struct ddekit_lock *ddekit_lock_t;

// common prototypes
static void ddekit_lock_init_locked(ddekit_lock_t *mtx);
static void ddekit_lock_init_unlocked(ddekit_lock_t *mtx);
#define ddekit_lock_init	ddekit_lock_init_unlocked
static void ddekit_lock_deinit  (ddekit_lock_t *mtx);
static void ddekit_lock_lock    (ddekit_lock_t *mtx);
static int  ddekit_lock_try_lock(ddekit_lock_t *mtx); // returns 0 on success, != 0 if it would block
static void ddekit_lock_unlock  (ddekit_lock_t *mtx);

// inline implementation or inline call to non-inline implementation
#include <l4/dde/ddekit/inline.h>

static INLINE void ddekit_lock_init_unlocked(ddekit_lock_t *mtx) {
	_ddekit_lock_init(mtx);
}

static INLINE void ddekit_lock_init_locked(ddekit_lock_t *mtx) {
	_ddekit_lock_init(mtx);
	_ddekit_lock_lock(mtx);
}

static INLINE void ddekit_lock_deinit(ddekit_lock_t *mtx) {
	_ddekit_lock_deinit(mtx);
}
static INLINE void ddekit_lock_lock(ddekit_lock_t *mtx) {
	_ddekit_lock_lock(mtx);
}
static INLINE int  ddekit_lock_try_lock(ddekit_lock_t *mtx) {
	return _ddekit_lock_try_lock(mtx);
}
static INLINE void ddekit_lock_unlock(ddekit_lock_t *mtx) {
	_ddekit_lock_unlock(mtx);
}

#endif
