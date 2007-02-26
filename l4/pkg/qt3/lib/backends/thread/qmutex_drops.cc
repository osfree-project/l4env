/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/backends/thread/qmutex_drops.cc
 * \brief  L4-specific QMutex implementation.
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

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/sys/syscalls.h>
#include <l4/semaphore/semaphore.h>

/*** type Q_MUTEX_T is needed by qmutex_p.h ***/
typedef l4semaphore_t Q_MUTEX_T;

/*** GENERAL INCLUDES ***/
#include <qmutex.h>
#include <private/qmutex_p.h>


#ifdef DEBUG
# define _DEBUG 1
#else
# define _DEBUG 0
#endif

#ifdef QT_THREAD_SUPPORT

/* **************************************************************** */
/* base destructor (QMutexPrivate is declared in qmutex_p.h) ****** */

QMutexPrivate::~QMutexPrivate() {

}

/* **************************************************************** */
/* DROPS-specific implementation ********************************** */
 
class QDropsMutexPrivate : public QMutexPrivate {
public:
  QDropsMutexPrivate(bool = false);
  
  void lock();
  void unlock();
  bool locked();
  bool trylock();
  int  type() const;

private:  
  int           count;     // lock() counter
  bool          recursive; 
  l4thread_t    owner;     // thread currently holding the lock
  l4semaphore_t myLock;    // protects 'count' for recursive locking
};



QDropsMutexPrivate::QDropsMutexPrivate(bool r) {

  recursive = r;
  count     = 0;
  handle    = L4SEMAPHORE_UNLOCKED;
  if (recursive)
    myLock = L4SEMAPHORE_UNLOCKED;
}


void QDropsMutexPrivate::lock() {

  if (recursive) {

    l4semaphore_down(&myLock);

    if (count > 0 && l4thread_equal(owner, l4thread_myself())) {
      count++;
    } else {
      l4semaphore_up(&myLock);
      l4semaphore_down(&handle);
      l4semaphore_down(&myLock);
      count = 1;
      owner = l4thread_myself();
    }

    l4semaphore_up(&myLock);

  } else {

    l4semaphore_down(&handle);
    count++;
  }
}


void QDropsMutexPrivate::unlock() {

  if (recursive) {

    l4semaphore_down(&myLock);

    if (l4thread_equal(owner, l4thread_myself())) {
      if (count > 0 && --count == 0) {
        count = 0;
        l4semaphore_up(&handle);
      }
    } else
      LOG("called by thread, which is not the current mutex owner");

    l4semaphore_up(&myLock);

  } else {
    count--;
    l4semaphore_up(&handle);
  }
}


bool QDropsMutexPrivate::locked() {

  int ret;

  if (recursive)
    l4semaphore_down(&myLock);

  if (l4semaphore_try_down(&handle)) {
    l4semaphore_up(&handle);
    ret = false;
  } else
    ret = true;

  if (recursive)
    l4semaphore_up(&myLock);

  return ret;
}


bool QDropsMutexPrivate::trylock() {

  int ret;

  if (recursive)
    l4semaphore_down(&myLock);

  if (l4semaphore_try_down(&handle)) {
    count = 1;
    owner = l4thread_myself();
    ret   = true;
  } else
    ret   = false;

  if (recursive)
    l4semaphore_up(&myLock);

  return ret;
}


int QDropsMutexPrivate::type() const {

  return recursive ? Q_MUTEX_RECURSIVE : Q_MUTEX_NORMAL;
}

/* **************************************************************** */
/* API class (declared in qmutex.h) ******************************* */

QMutex::QMutex(bool recursive) {

  d = new QDropsMutexPrivate(recursive);
}

QMutex::~QMutex() {

  delete d;
}

void QMutex::lock() {

  d->lock();
}

void QMutex::unlock() {

  d->unlock();
}

bool QMutex::locked() {

  return d->locked();
}

bool QMutex::tryLock() {

  return d->trylock();
}

#endif // QT_THREAD_SUPPORT
