/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>

int qt_log_drops(char *buf);
int qt_log_drops(char *buf)
{
  LOG("%s", buf);
  return 0;
}
  
