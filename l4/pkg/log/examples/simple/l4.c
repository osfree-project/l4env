/*! 
 * \file   log/examples/simple/l4.c
 * \brief  Show the use of an own output function. 
 *
 * \date   09/15/1999
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * The old function is stored in the old_outfunc-var. It will be called
 * later with the modified text.
 */

#include <flux/c/stdio.h>
#include <flux/c/ctype.h>
#include <l4/log/l4log.h>

char LOG_tag[9]=LOG_TAG;

/* Just an example */
void (*old_outfunc)(const char*);
void outstring(const char*s);
void outstring(const char*s){
  char *c=(char*)s;
  
  while((*c=toupper(*c))!=0)c++;
  old_outfunc(s);
}

/* for static reinitialization remove the leading // next line */
//void (*LOG_outstring)(char*)=outstring;

int main(void){
  LOG("simple LOG call, decimal arg: %d",1);
  LOGl("call to LOGl, decimal arg: %d",2);
  LOGL("call to LOGL, decimal arg: %d",3);
  old_outfunc=LOG_outstring;
  LOG_outstring=outstring;
  LOG("simple LOG call, decimal arg: %d",4);
  LOGl("call to LOGl, decimal arg: %d",5);
  LOGL("call to LOGL, decimal arg: %d",6);

  return 0;
}
