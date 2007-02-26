/*!
 * \file   log/examples/l4env-printf/l4env.c
 * \brief  Example for printf-usage with l4env.
 *
 * \date   01/15/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * We declare the LOG_tag variable to set the logtag. The
 * loglib-initialization is no longer needed.
 *
 * The logtag will be printed in front of each line put out.
 */

#include <l4/log/l4log.h>
#include <stdio.h>

char LOG_tag[9] = LOG_TAG;

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
