/* $Id$ */
/*****************************************************************************/
/**
 * \file   examples/thread_test/main.cc
 * \brief  Thread subsystem test case.
 *
 * \date   07/13/2004
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2006 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

#include <qmutex.h>
#include <qwaitcondition.h>
#include <qthread.h>
#include <qthreadstorage.h>

#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/thread/thread.h>

/* ************************************************************ */

#define TEST_RECURSIVE 0

/* ************************************************************ */

QMutex                *m;
QWaitCondition        *cond;
QThreadStorage<int *> *ths;

/* ************************************************************ */

class MyThread: public QThread {

  void run();
};


void MyThread::run() {

  int *me = new int;

  *me = (int) QThread::currentThread();
  ths->setLocalData(me);


  // QThread doesn't provide a method to get a thread's scheduling
  // priority, so we ask l4thread ...
  LOG("[%d] My priority is %d", *ths->localData(), l4thread_get_prio(l4thread_myself()));

  LOG("[%d] Do cpu demanding things (busy waiting) ...", *ths->localData());
  int i, var = *me;
  for (i = 0; i < 2000 * 1048576; i++)
    var++;
  LOG("[%d] ... stopped busy waiting (%d iterations)", *ths->localData(), var - *me);


  // test QMutex implementation
  LOG("[%d] Testing QMutex", *ths->localData());
#if TEST_RECURSIVE
  m->lock();
#endif
  m->lock();
  LOG("[%d] locked ...", *ths->localData());
#if TEST_RECURSIVE
  m->unlock();
#endif
  QThread::msleep(1000);
  m->unlock();
  i = *ths->localData();
  LOG("[%d] ... unlocked", *ths->localData());

  // test QWaitCondition implementation
  LOG("[%d] Testing QWaitCondition", *ths->localData());

  LOG("[%d] wait()ing ...", *ths->localData());
  cond->wait();
  LOG("[%d] ... I'm back!", *ths->localData());

  QThread::msleep(500);
  LOG("[%d] Falling asleep now ...", *ths->localData());  
}

/* ************************************************************ */

int main(int argc, char **argv) {

  // using QThreadStorage for storing our own thread ID may seem to be
  // overkill,but it is a nice testcase ...
  ths = new QThreadStorage<int *>;

#if TEST_RECURSIVE
  m    = new QMutex(true);
#else
  m    = new QMutex;
#endif
  cond = new QWaitCondition;

  // create test threads
  MyThread th0, th1, th2;
  th0.start(QThread::LowestPriority);
  th1.start(QThread::NormalPriority);
  th2.start(QThread::HighestPriority);

  int *me = new int;
  *me = (int) QThread::currentThread();
  ths->setLocalData(me);

  // test QWaitCondition::wake{One|All}()
  l4_sleep(6000);
  LOG("[%d] waking one thread", *ths->localData());
  cond->wakeOne();

  l4_sleep(1000);
  LOG("[%d] waking remaining threads", *ths->localData());
  cond->wakeAll();

  l4_sleep_forever();
  return 0;  
}

