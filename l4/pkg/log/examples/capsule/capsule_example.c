/*!
 * \file   log/examples/capsule/capsule_example.c
 * \brief  Example for printf- and LOG-makro usage, without referencing printf
 *
 * \date   02/14/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * The log tag is specified either by the LOG_tag variable or by calling
 * LOG_init().
 */
#include <l4/log/l4log.h>

char LOG_tag[9]=LOG_TAG;

int main(void){
  int loop=0;
  
  for(loop=0;;loop++){
    LOG("Hi. This is loop #%d", loop);
    LOG("Ok, here we present a fairly long string, which should be broken "
           "from the logging library. The printf-implementation uses "
           "a buffer, and if it is filled the message is flushed.");
    LOG("and this is LOG");
    LOGl("and this is LOGl");
    LOGL("to come to an end, this is LOGL");
  }
  return 0;
}
