/*!
 * \file   log/lib/src/log_init.c
 * \brief  LOG_init function
 *
 * \date   02/13/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
#include <l4/log/l4log.h>
#include <string.h>
char LOG_tag[9] __attribute__((weak));
char LOG_tag[9]="liblog";

void LOG_init(const char*s){
    memset(LOG_tag, ' ',sizeof(LOG_tag)-1);
    strncpy(LOG_tag, s, sizeof(LOG_tag));
    LOG_tag[sizeof(LOG_tag)-1]=0;
}
