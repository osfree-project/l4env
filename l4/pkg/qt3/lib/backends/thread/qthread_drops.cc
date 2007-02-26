/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/backends/thread/qthread_drops.cc
 * \brief  L4-specific QThread implementation.
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

// this file is based on qthread_unix.cpp

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/semaphore/semaphore.h>
#include <l4/thread/thread.h>

/*** type Q_MUTEX_T is needed by qmutex_p.h ***/
typedef l4semaphore_t Q_MUTEX_T;

/*** GENERAL INCLUDES ***/
#include <qthread.h>
#include <qthreadstorage.h>
#include <private/qthreadinstance_p.h>
#include <private/qmutexpool_p.h>


#ifdef DEBUG
# define _DEBUG 1
#else
# define _DEBUG 0
#endif

#ifdef QT_THREAD_SUPPORT

/* **************************************************************** */

 // key to access thread local pointer to QThreadInstance
static int thiKey = -1;

static QMutexPool      *qt_thread_mutexpool = 0;
static QThreadInstance *mainInstance;

/* **************************************************************** */

// allocates a certain key at most once
static inline void allocateKeyOnce(int *key) {
  if (*key == -1)
    *key = l4thread_data_allocate_key();
  if (*key == -L4_ENOKEY)
    LOG_Error("unable to allocate key for thread-local data");
}

/* **************************************************************** */
/* partly L4-specific data, which belongs to QThreadInstance ****** */

class QThreadInstanceData {
public:
  QWaitCondition       threadDone;
  l4thread_t           threadId;
  l4thread_exit_desc_t finish_desc;
  bool                 terminated;
};

/* **************************************************************** */
/* DROPS-specific implementation of QThreadInstance *************** */ 

// wrapper for L4 thread library
void qthread_instance_finish(l4thread_t thread, void *data) {

  QThreadInstance *thi = (QThreadInstance *) data;
  QThreadInstance::finish(thi);
}


QThreadInstance *QThreadInstance::current() {

  allocateKeyOnce(&thiKey);
  QThreadInstance *ret = (QThreadInstance *) l4thread_data_get_current(thiKey);
  return ret;
}


void QThreadInstance::init(unsigned int stackSize) {

  stacksize      = (stackSize == 0) ? L4THREAD_DEFAULT_SIZE : stackSize;
  args[0]        = args[1] = NULL;
  thread_storage = 0;
  finished       = false;
  running        = false;
  orphan         = false;
  
  data             = new QThreadInstanceData;
  data->threadId   = L4THREAD_INVALID_ID;
  data->terminated = false;
  
  // threads have not been initialized yet, do it now
  if ( !qt_thread_mutexpool)
    QThread::initialize();
}


void QThreadInstance::deinit() {

  delete data;
}


void QThreadInstance::start(void *_arg)
{
  void           **arg = (void **) _arg;
  QThreadInstance *d   = (QThreadInstance *) arg[1];

  allocateKeyOnce(&thiKey);
  l4thread_data_set_current(thiKey, d);

  d->data->finish_desc.fn = qthread_instance_finish;
  l4thread_on_exit(&d->data->finish_desc, d);

  ((QThread *) arg[0])->run();  
}


void QThreadInstance::finish(QThreadInstance *d) {

  if (d == NULL) {
    LOG_Error("QThread: internal error: zero data for running thread.");
    return;
  }

  // If this thread was killed using QThread::terminate(), we must not
  // grab the mutex in order to avoid deadlocking. The mutex is
  // already held by the thread, which killed us. Since this one
  // blocks in l4thread_shutdown() until we are finished, it is safe
  // to cleanup now.
  bool terminated = d->data->terminated;
  if ( !terminated)
    d->mutex()->lock();

  d->running  = false;
  d->finished = true;
  d->args[0]  = d->args[1] = NULL;

  QThreadStorageData::finish(d->thread_storage);
  d->thread_storage = 0;

  if (d->data) {
    d->data->threadId = L4THREAD_INVALID_ID;
    d->data->threadDone.wakeAll();
    delete d->data;
    d->data = NULL;
  }

  if (d->orphan) {
    d->deinit();
    delete d;
  }

  if ( !terminated)
    d->mutex()->unlock();
}


QMutex *QThreadInstance::mutex() const {

  return qt_thread_mutexpool ? qt_thread_mutexpool->get((void *) this) : NULL;
}


void QThreadInstance::terminate() {

  if (data->threadId == L4THREAD_INVALID_ID)
    return;

  // QThreadInstance::finish() evaluates 'data->terminated' in order
  // to find out, whether the thread has been shot down via
  // QThread::terminate(). See also QThreadInstance::finish().
  data->terminated = true;
  l4thread_shutdown(data->threadId);
}

/* **************************************************************** */
/* DROPS-specific implementation of QThread *********************** */ 

Qt::HANDLE QThread::currentThread() {

  return (HANDLE) l4thread_myself();
}


void QThread::initialize()
{
  if (qt_global_mutexpool == NULL)
    qt_global_mutexpool = new QMutexPool(true, 73);

  if (qt_thread_mutexpool == NULL)
    qt_thread_mutexpool = new QMutexPool(false, 127);
  
  // create a QThreadInstance for the current thread, the one which
  // was started in main() by the OS
  mainInstance = new QThreadInstance;
  mainInstance->init(0);

  allocateKeyOnce(&thiKey);
  l4thread_data_set_current(thiKey, mainInstance);
}


void QThread::cleanup() {

  delete qt_global_mutexpool;
  delete qt_thread_mutexpool;
  qt_global_mutexpool = 0;
  qt_thread_mutexpool = 0;
  
  QThreadInstance::finish(mainInstance);

  allocateKeyOnce(&thiKey);
  l4thread_data_set_current(thiKey, NULL);
}


void QThread::exit() {

  l4thread_exit();
}


void QThread::sleep(unsigned long secs) {

  l4thread_sleep(secs * 1000);
}


void QThread::msleep(unsigned long msecs) {

  l4thread_sleep(msecs);
}


void QThread::usleep(unsigned long usecs) {

  l4thread_usleep(usecs);
}


void QThread::start(Priority priority)
{
  QMutexLocker locker(d->mutex());

  if (d->running)
    d->data->threadDone.wait();
  d->running  = true;
  d->finished = false;
  
  int       ret;
  l4_prio_t prio;

  switch (priority) {
  case InheritPriority:
    prio = l4thread_get_prio(l4thread_myself());
    break;
  case IdlePriority:
    prio = l4thread_default_prio - 3;
    break;
  case LowestPriority:
    prio = l4thread_default_prio - 2;
    break;
  case LowPriority:
    prio = l4thread_default_prio - 1;
    break;
  case NormalPriority:
    prio = l4thread_default_prio;
    break;    
  case HighPriority:
    prio = l4thread_default_prio + 1;
    break;
  case HighestPriority:
    prio = l4thread_default_prio + 2;
    break;
  case TimeCriticalPriority:
    prio = l4thread_default_prio + 3;
    break;
  default:
    LOG("Invalid priority, using L4THREAD_DEFAULT_PRIO");
    prio = L4THREAD_DEFAULT_PRIO;
    break;
  }

  if (d->stacksize <= 0 && d->stacksize != L4THREAD_DEFAULT_SIZE) {
    LOG("invalid stack size: %d", d->stacksize);
    d->running  = false;
    d->finished = false;
    return;
  }

  d->args[0] = this;
  d->args[1] = d;

  ret = l4thread_create_long(L4THREAD_INVALID_ID, QThreadInstance::start, NULL,
                             L4THREAD_INVALID_SP, d->stacksize, prio, d->args,
                             L4THREAD_CREATE_ASYNC);
  if (ret < 0) {

    if (ret == -L4_ENOMEM || ret == -L4_ENOMAP)
      LOG("QThread::start: failed to allocate stack");
    if (ret == -L4_ENOTHREAD)
      LOG("QThread::start: failure: max. number of threads reached");

    d->running  = false;
    d->finished = false;
    d->args[0]  = d->args[1] = NULL;

  } else
    d->data->threadId = ret;
}


void QThread::start() {

  start(InheritPriority);
}


bool QThread::wait(unsigned long time)
{
  QMutexLocker locker( d->mutex() );

  if (d->data->threadId == l4thread_myself()) {
    LOGd(_DEBUG, "QThread::wait: thread tried to wait on itself");
    return false;
  }

  if (d->finished || !d->running)
    return true;

  return d->data->threadDone.wait(time);
}


#endif // QT_THREAD_SUPPORT
