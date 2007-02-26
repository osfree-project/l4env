#ifndef L4_L4IOSTREAM_H__
#define L4_L4IOSTREAM_H__

#include <l4/cxx/iostream.h>
#include <l4/sys/types.h>

inline L4::BasicOStream &operator << ( L4::BasicOStream &s, l4_threadid_t id )
{
  L4::IOBackend::Mode m = s.get_be_mode();
  s << L4::dec << id.id.task << "." << id.id.lthread;
  s.restore_be_mode(m);
  return s;
}


#endif
