#include <l4/log/l4log.h>
#include "uip.h"

// no logging per default - the uIP lib is too talkative
void uip_log(char *m)
{
//          LOG("%s", m);
}

