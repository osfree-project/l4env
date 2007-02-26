/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/sys/syscalls.h>
#include <l4/semaphore/semaphore.h>

/*** type Q_MUTEX_T is needed by qmutex_p.h ***/
typedef l4semaphore_t Q_MUTEX_T;

/*** GENERAL INCLUDES ***/
#include <qmutex.h>
#include <private/qmutex_p.h>
#include <qwaitcondition.h>


#ifdef DEBUG
# define _DEBUG 1
#else
# define _DEBUG 0
#endif

#ifdef QT_THREAD_SUPPORT

/* **************************************************************** */

class QWaitConditionPrivate {
public:
  int           numWaiters; // number of threads currently wait()ing
  l4semaphore_t waiterSem;  // used to suspend/wake threads
  l4semaphore_t myLock;     // protects numWaiters
  QMutex        myMutex;    // provided by QWaitCondition for cenvenience
};

/* **************************************************************** */
 
QWaitCondition::QWaitCondition() {

  d = new QWaitConditionPrivate;

  d->numWaiters = 0;
  d->waiterSem  = L4SEMAPHORE_LOCKED;
  d->myLock     = L4SEMAPHORE_UNLOCKED;
}


QWaitCondition::~QWaitCondition() {

}


void QWaitCondition::wakeOne() {

  l4semaphore_down(&d->myLock);

  if (d->numWaiters > 0) {
    d->numWaiters--;
    l4semaphore_up(&d->waiterSem);
  }

  l4semaphore_up(&d->myLock);
}


void QWaitCondition::wakeAll() {

  l4semaphore_down(&d->myLock);

  while (d->numWaiters > 0) {
    d->numWaiters--;
    l4semaphore_up(&d->waiterSem);
  }

  l4semaphore_up(&d->myLock);
}


bool QWaitCondition::wait(unsigned long time) {

  QMutexLocker locker(&d->myMutex);

  return wait(&d->myMutex, time);
}


bool QWaitCondition::wait(QMutex *mutex, unsigned long time) {

  if (mutex == NULL) {
    LOGd(_DEBUG, "mutex == NULL");
    return false;
  }

  // behave as described in the documentation ...
  if (mutex->d->type() == Q_MUTEX_RECURSIVE) {
    LOGd(_DEBUG, "mutex is of recursive type, this is undefined");
    return false;
  }
  if (mutex->locked() == false) {
    LOGd(_DEBUG, "mutex is not locked");
    return false;
  }

  int ret = 0;

  // atomically unlock mutex and enqueue into wait() queue
  l4semaphore_down(&d->myLock);

  mutex->unlock();
  d->numWaiters++;

  l4semaphore_up(&d->myLock);

  // wait ...
  if (time < ULONG_MAX)
    ret = l4semaphore_down_timed(&d->waiterSem, time);
  else
    l4semaphore_down(&d->waiterSem);

  mutex->lock();

  return ret == 0;
}

#endif // QT_THREAD_SUPPORT

