#include <l4/dde/ddekit/semaphore.h>
#include <l4/dde/ddekit/memory.h>

#include <l4/semaphore/semaphore.h>

struct ddekit_sem {
	l4semaphore_t sem;
};

ddekit_sem_t *ddekit_sem_init(int value) {
	ddekit_sem_t *sem;

	sem = (ddekit_sem_t *) ddekit_simple_malloc(sizeof(*sem));
	sem->sem = L4SEMAPHORE_INIT(value);

	return sem;
}

void ddekit_sem_deinit(ddekit_sem_t *sem) {
	ddekit_simple_free(sem);
}

void ddekit_sem_down(ddekit_sem_t *sem) {
/*printf("%s:%d sem=%p l4sem=0x%08x\n", __FILE__, __LINE__, sem, sem->sem); */
/*	enter_kdebug(""); */
	l4semaphore_down(&sem->sem);
/*printf("%s:%d\n", __FILE__, __LINE__); */
}

/* returns 0 on success, != 0 when it would block */
int  ddekit_sem_down_try(ddekit_sem_t *sem) {
	return l4semaphore_try_down(&sem->sem) ? 0 : 1;
}

/* returns 0 on success, != 0 on timeout */
int  ddekit_sem_down_timed(ddekit_sem_t *sem, int timo) {
	return l4semaphore_down_timed(&sem->sem, timo);
}

void ddekit_sem_up(ddekit_sem_t *sem) {
	l4semaphore_up(&sem->sem);
}

