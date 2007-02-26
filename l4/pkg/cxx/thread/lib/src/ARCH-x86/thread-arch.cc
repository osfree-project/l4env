#include <l4/cxx/thread.h>
#include <l4/util/util.h>


namespace L4 {

  void Thread::start_cxx_thread(Thread *_this)
  {
    _this->execute();
  }
};

