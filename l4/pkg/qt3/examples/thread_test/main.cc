#include <qmutex.h>
#include <qwaitcondition.h>
#include <qthread.h>
#include <qthreadstorage.h>

#include <l4/log/l4log.h>
#include <l4/util/util.h>

/* ************************************************************ */

#define TEST_RECURSIVE 0

/* ************************************************************ */

QMutex         *m;
QWaitCondition *cond;

/* ************************************************************ */

class MyThread: public QThread {

  void run();
};


void MyThread::run() {

  QThreadStorage<int *> ths;
  int *tmp = new int;

  *tmp = (int) QThread::currentThread();
  ths.setLocalData(tmp);


  LOG("[%d] Testing QMutex", *ths.localData());

#if TEST_RECURSIVE
  m->lock();
#endif
  m->lock();
  LOG("[%d] locked ...", *ths.localData());
#if TEST_RECURSIVE
  m->unlock();
#endif
  QThread::msleep(500);
  LOG("[%d] ... unlocked", *ths.localData());
  m->unlock();


  LOG("[%d] Testing QWaitCondition", *ths.localData());

  LOG("[%d] wait()ing ...", *ths.localData());
  QMutexLocker locker(m);
  cond->wait(m);
  LOG("[%d] ... I'm back!", *ths.localData());

  QThread::msleep(500);
  LOG("[%d] Falling asleep now ...", *ths.localData());  
}

/* ************************************************************ */

int main(int argc, char **argv) {

#if TEST_RECURSIVE
  m    = new QMutex(true);
#else
  m    = new QMutex;
#endif
  cond = new QWaitCondition;

  // create test threads
  MyThread th0, th1, th2;
  th0.start();
  th1.start();
  th2.start();

  int me = (int) QThread::currentThread();

  // test QWaitCondition::wake{One|All}()
  l4_sleep(2000);
  LOG("[%d] waking one thread", me);
  cond->wakeOne();
  LOG("[%d] killing one thread (th2)", me);
  th2.terminate();
  l4_sleep(1000);
  LOG("[%d] waking remaining threads", me);
  cond->wakeAll();

  l4_sleep_forever();
  return 0;  
}

