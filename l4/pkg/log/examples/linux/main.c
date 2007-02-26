/*!
 * \file   log/examples/linux/main.c
 * \brief  Example showing how to use loglib with Linux.
 *
 * \date   02/14/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * This program accepts one cmdline-arg, which is assumed to be the name
 * of a file, where the log-output is written to. If no name is given,
 * output is delivered to stderr.
 */
#include <l4/log/l4log.h>
#include <stdio.h>
#include <stdlib.h>

char LOG_tag[9]=LOG_TAG;

/* We have no prototype exported but know about it. */
extern FILE *LOG_stdlog;

int main(int argc, char**argv){
  /* You do not need the following, but you can use this to redirect
     your log output. */
  if(argc>1){
  	if((LOG_stdlog = fopen(argv[1], "w"))==0){
            perror(argv[1]);
            exit(1);
        }
  }
  
  printf("Hi, thats printf\n");
  printf("Ok, here we present a fairly long string, which is not split "
         "by the logging library, as we do not use its printf "
         "but the standard printf of the libc of Linux.\n");
  LOG("and this is LOG");
  LOG("This is LOG again, this time with a long string, which is expected "
      "to be split by LOG_printf. "
      "The LOG macros use their own function to operate even in an environment "
      "with a broken printf().");
  LOGl("and this is LOGl");
  LOGL("to come to an end, this is LOGL");

  LOG("And finaly, a test with %%n: %n. Ok.", (unsigned*)3);

  return 0;
}
