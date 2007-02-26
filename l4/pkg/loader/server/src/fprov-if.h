#ifndef LOADER_FPROV_IF_H
#define LOADER_FPROV_IF_H

#include <l4/l4rm/l4rm.h>

extern l4_threadid_t tftp_id;

int load_file(const char *fname, l4_threadid_t fprov_id, l4_threadid_t dm_id,
	      int use_modpath, int contiguous, 
	      l4_addr_t *addr, l4_size_t *size, l4dm_dataspace_t *ds);

int fprov_if_init(void);

#endif

