/* -*- c++ -*- */
#ifndef L4_CXX_TYPES_H__
#define L4_CXX_TYPES_H__

#include <l4/sys/types.h>
#include <l4/cxx/iostream.h>

namespace L4 {

  class MsgDope 
  {
  public:
    MsgDope( l4_msgdope_t md ) : _md(md) {}
    MsgDope( l4_umword_t _raw ) : _md((l4_msgdope_t){raw: _raw}) {}

    bool msg_deceited() const { return _md.md.msg_deceited; }
    bool fpage_received() const { return _md.md.fpage_received; }
    bool msg_redirected() const { return _md.md.msg_redirected; }
    unsigned error_code() const { return _md.raw & 0x0f0; }
    bool send_error() const { return _md.md.snd_error; }
    unsigned strings() const { return _md.md.strings; }
    unsigned words() const { return _md.md.words; }

    operator l4_msgdope_t () { return _md; }
    operator l4_umword_t () { return _md.raw; }

  private:
    l4_msgdope_t _md;
  };

  extern char *ipc_error_str[];

};

L4::BasicOStream &operator << (L4::BasicOStream &o, L4::MsgDope md)
{
  if(md.error_code())
    {
      if(md.error_code()==0x10) 
        {
          o << "receiver not existent";
          return o;
        }
      if(md.send_error())
        o << "send ";
      else
        o << "receive ";
      
      o << L4::ipc_error_str[md.error_code()>>5];
    }
  else
    o << "ok";
  
  return o;
}

#endif /* L4_CXX_TYPES_H__ */
