#include "test-server.h"
#include <l4/log/l4log.h>

CORBA_int pf_handler(CORBA_Object src, 
    handler_msg_buffer_t* msg_buffer,
    CORBA_Server_Environment *env)
{
  l4_umword_t pfa, eip;
  l4_snd_fpage_t page;
  unsigned char ro_or_rw = 0;
  long d;
  // get pfa and eip
  pfa = DICE_GET_DWORD(msg_buffer, 0);
  eip = DICE_GET_DWORD(msg_buffer, 1);
  LOG("recv PF at 0x%x, eip 0x%x from %x.%x",
      pfa, eip, src->id.task, src->id.lthread);
  // check fro rw/ro
  if (pfa & 2)
    ro_or_rw = L4_FPAGE_RW;
  else
    ro_or_rw = L4_FPAGE_RO;
  // touch there myself, so I get it
  d = *(long*)pfa;
  // calculate page (can use src to determine client)
  // (for this example, we simply getthe page of the PF-Address
  //  and send this page back to the user)
  pfa &= ~0xfff;
  page.snd_base = pfa;
  page.fpage = l4_fpage(pfa, 12, ro_or_rw, L4_FPAGE_MAP);
  // marshal fpage
  DICE_MARSHAL_FPAGE(msg_buffer, page, 0);
  DICE_SET_SHORTIPC_COUNT(msg_buffer);
  DICE_SET_SEND_FPAGE(msg_buffer);
  // return that server should reply
  return DICE_REPLY;
}
                                          
