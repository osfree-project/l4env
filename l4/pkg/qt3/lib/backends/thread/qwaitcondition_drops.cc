/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/backends/thread/qwaitcondition_drops.cc
 * \brief  L4-specific QWaitCondition implementation.
 *
 * \date   11/02/2004
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2006 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

/*
 * semaphore logic borrowed from libSDL: src/thread/generic/SDL_syscond.c
 */

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
  int           numSignals; // number of threads that were woken up
  l4semaphore_t waiterSem;  // used to suspend/wake threads
  l4semaphore_t signalerSem;// used to block the signaler until the
                            // woken threads say good bye
  l4semaphore_t myLock;     // protects numWaiters, numSignals
  QMutex        myMutex;    // provided by QWaitCondition for cenvenience
};

/* **************************************************************** */
 
QWaitCondition::QWaitCondition() {

  d = new QWaitConditionPrivate;

  d->numWaiters  = 0;
  d->numSignals  = 0;
  d->waiterSem   = L4SEMAPHORE_LOCKED;
  d->signalerSem = L4SEMAPHORE_LOCKED;
  d->myLock      = L4SEMAPHORE_UNLOCKED;
}


QWaitCondition::~QWaitCondition() {

  delete d;
}


void QWaitCondition::wakeOne() {

  l4semaphore_down(&d->myLock);

  if (d->numWaiters > d->numSignals) {
    d->numSignals++;
    l4semaphore_up(&d->waiterSem);
    l4semaphore_up(&d->myLock);
    l4semaphore_down(&d->signalerSem);
  } else
    l4semaphore_up(&d->myLock);
}


void QWaitCondition::wakeAll() {

  l4semaphore_down(&d->myLock);

  int numWaiting = d->numWaiters - d->numSignals;
  if (numWaiting > 0) {
    d->numSignals = d->numWaiters;
    
    int i;
    for (i = 0; i < numWaiting; i++)
      l4semaphore_up(&d->waiterSem);

    l4semaphore_up(&d->myLock);
    for (i = 0; i < numWaiting; i++)
      ;//l4semaphore_down(&d->signalerSem);
      
  } else
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

  // atomically unlock mutex and enqueue into wait() queue
  l4semaphore_down(&d->myLock);

  mutex->unlock();
  d->numWaiters++;

  l4semaphore_up(&d->myLock);

  int ret = 0;

  // wait ...
  if (time < ULONG_MAX)
    ret = l4semaphore_down_timed(&d->waiterSem, time);
  else
    l4semaphore_down(&d->waiterSem);

  l4semaphore_down(&d->myLock);
  if (d->numSignals > 0) {
    if (ret != 0)
      l4semaphore_down(&d->waiterSem);
    l4semaphore_up(&d->signalerSem);
    d->numSignals--;    
  }
  d->numWaiters--;
  l4semaphore_up(&d->myLock);
  
  mutex->lock();

  return ret == 0;
}

#endif // QT_THREAD_SUPPORT

