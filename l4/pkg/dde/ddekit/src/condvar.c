/**
 * Unchecked (no BSD invariants) condition variable implementation for
 * dde-internal use. Written from scratch.
 *
 * \author Thomas Friebel <tf13@os.inf.tu-dresden.de>
 */
#include <l4/dde/ddekit/condvar.h>
#include <l4/dde/ddekit/lock.h>
#include <l4/dde/ddekit/memory.h>

#include <l4/log/l4log.h>
#include <l4/semaphore/semaphore.h>
#include <l4/lock/lock.h>

struct ddekit_condvar {
	unsigned waiters;
	unsigned signals;
	l4lock_t lock;
	l4semaphore_t sem;
	l4semaphore_t handshake;
};

ddekit_condvar_t *ddekit_condvar_init() {
	ddekit_condvar_t *cvp;

	cvp = ddekit_simple_malloc(sizeof(*cvp));

	cvp->waiters   = 0;
	cvp->signals   = 0;
	cvp->lock      = L4LOCK_UNLOCKED;
	cvp->sem       = L4SEMAPHORE_INIT(0);
	cvp->handshake = L4SEMAPHORE_INIT(0);

	return cvp;
}

void ddekit_condvar_wait(ddekit_condvar_t *cvp, ddekit_lock_t *mp) {
	ddekit_condvar_wait_timed(cvp, mp, -1);
}

int ddekit_condvar_wait_timed(ddekit_condvar_t *cvp, ddekit_lock_t *mp, int timo) {
	int rval;

	l4lock_lock(&cvp->lock);
	cvp->waiters++;
	l4lock_unlock(&cvp->lock);

	ddekit_lock_unlock(mp);

	if (timo == -1) {
		l4semaphore_down(&cvp->sem);
		rval = 0;
	} else {
		rval = l4semaphore_down_timed(&cvp->sem, timo);
	}

	l4lock_lock(&cvp->lock);
	if (cvp->signals > 0) {
		/* if we timed out, but there is a signal now, consume it */
		if (rval) l4semaphore_down(&cvp->sem);

		l4semaphore_up(&cvp->handshake);
		cvp->signals--;
	}
	cvp->waiters--;
	l4lock_unlock(&cvp->lock);

	ddekit_lock_lock(mp);

	return rval;
}

void ddekit_condvar_signal(ddekit_condvar_t *cvp)
{
	l4lock_lock(&cvp->lock);

	if (cvp->waiters > cvp->signals) {
		cvp->signals++;
		l4semaphore_up(&cvp->sem);
		l4lock_unlock(&cvp->lock);
		l4semaphore_down(&cvp->handshake);
	} else {
		/* nobody left to wakeup */
		l4lock_unlock(&cvp->lock);
	}
}

void ddekit_condvar_broadcast(ddekit_condvar_t *cvp) {
	int waiters;

	l4lock_lock(&cvp->lock);

	waiters = cvp->waiters - cvp->signals;
	if (waiters > 0) {
		int i;

		cvp->signals = cvp->waiters;
		for (i=0; i<waiters; i++) {
			l4semaphore_up(&cvp->sem);
		}
		l4lock_unlock(&cvp->lock);
		for (i=0; i<waiters; i++) {
			l4semaphore_down(&cvp->handshake);
		}
	} else {
		l4lock_unlock(&cvp->lock);
	}
}
