#include <l4/cxx/iostream.h>
#include <l4/sys/kdebug.h>

namespace L4 {

  class KdbgIOBackend : public IOBackend
  {
  protected:
    void write(char const *str, unsigned len);
  };
  
  void KdbgIOBackend::write(char const *str, unsigned len)
  {
    outnstring(str,len);
  }

  namespace {
    KdbgIOBackend iob;
  };
  
  BasicOStream cout(&iob);
  BasicOStream cerr(&iob);
};
