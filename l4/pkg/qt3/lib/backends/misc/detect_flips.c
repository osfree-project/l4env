/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>

static int flips_available = -1;

int l4_qt_flips_available(void);
int l4_qt_flips_available(void) {

  l4_threadid_t flips_id;

  if (flips_available == -1) {

    /* check, whether FLIPS is registered at names and cache the result */
    flips_available = names_query_name("FLIPS", &flips_id);
    if ( !flips_available)
      LOG("IP Stack not registered at names! IP sockets unavailable!");
  }

  return flips_available;
}
