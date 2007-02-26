/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>

#include <l4/qt3/l4qws_qlock-client.h>

l4qws_key_t l4qws_key(const char *fn, char c) {

  // fn is supposed to point to the same file (local socket)
  return 42 + c;
}


