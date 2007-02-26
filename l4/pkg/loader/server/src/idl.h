#ifndef __LOADER_IDL_H
#define __LOADER_IDL_H

#include "loader-server.h"

l4_int32_t 
l4loader_bin_server_open(sm_request_t *request, const char *fname,
			 const l4loader_threadid_t *dm, l4_uint32_t flags,
			 sm_exc_t *_ev);
void server_loop(void) __attribute__ ((noreturn));

#endif

