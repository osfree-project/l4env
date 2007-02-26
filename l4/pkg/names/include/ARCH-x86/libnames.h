#ifndef __LIBNAMES_H
#define __LIBNAMES_H

#include <l4/sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif
  
int names_register(const char* name);
int names_unregister(const char* name);
int names_query_name(const char* name, l4_threadid_t* id);
int names_query_id(const l4_threadid_t id, char* name, const int length);
int names_waitfor_name(const char* name, l4_threadid_t* id, const int timeout);
int names_query_nr(int nr, char* name, int length, l4_threadid_t *id);
int names_unregister_task(l4_threadid_t tid);
  
#ifdef __cplusplus
}
#endif

#endif
