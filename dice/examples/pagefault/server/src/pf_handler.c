#include "test-server.h"
#include <l4/log/l4log.h>

CORBA_int pf_handler(CORBA_Object *src, handler_msg_buffer_t* msg_buffer,
        CORBA_Environment *env)
{
  l4_umword_t pfa, eip;
  l4_snd_fpage_t page;
  unsigned char ro_or_rw = 0;
  long d;
  // get pfa and eip
  pfa = GET_DWORD(msg_buffer, 0);
  eip = GET_DWORD(msg_buffer, 1);
  LOG("rcv pf (0x%x, 0x%x)", pfa, eip);
  // check fro rw/ro
  if (pfa & 2)
    ro_or_rw = L4_FPAGE_RW;
  else
    ro_or_rw = L4_FPAGE_RO;
  // touch there myself, so I get it
  d = *(long*)pfa;
  LOG("read %d at 0x%x", d, pfa);
  // calculate page (can use src to determine client)
  // (for this example, we simply getthe page of the PF-Address
  //  and send this page back to the user)
  pfa &= ~0xfff;
  page.snd_base = pfa;
  page.fpage = l4_fpage(pfa, 12, ro_or_rw, L4_FPAGE_MAP);
  // marshal fpage
  MARSHAL_FPAGE(msg_buffer, page, 0);
  SET_SHORTIPC_COUNT(msg_buffer);
  SET_SEND_FPAGE(msg_buffer);
  // return that server should reply
  return REPLY;
}
                                          
