/*!
 * \file   log/examples/printf/l4.c
 * \brief  Example for printf- and LOG-makro usage.
 *
 * \date   09/15/1999
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * The log tag is specified either by the LOG_tag variable or by calling
 * LOG_init().
 */
#include <l4/log/l4log.h>
#include <stdio.h>

char LOG_tag[9]=LOG_TAG;

int main(void){
  int loop=0;
  
  for(loop=0;;loop++){
    printf("Hi, thats printf (#%d)\n", loop);
    printf("Ok, here we present a fairly long string, which should be broken "
           "from the logging library. The printf-implementation uses "
           "a buffer, and if it is filled the message is flushed.\n");
    LOG("and this is LOG");
    LOGl("and this is LOGl");
    LOGL("to come to an end, this is LOGL");
  }
  return 0;
}
