/*
 * \brief  Lib-internal data structures
 * \author Norman Feske
 * \date   2007-06-06
 */

#ifndef _NITPICKER_LIB_NITPICKER_PRIVATE_H_
#define _NITPICKER_LIB_NITPICKER_PRIVATE_H_

#include <l4/dm_mem/dm_mem.h>
#include <l4/sys/types.h>

struct nitpicker {
	l4_threadid_t srv;
};

struct nitpicker_buffer {
	l4dm_dataspace_t ds;
	void             *addr;
	int               id;
	unsigned int      width;
	unsigned int      height;
	struct nitpicker *np;
};

struct nitpicker_view {
	struct nitpicker_buffer *npb;
	l4_threadid_t            listener;
	int                      id;
};

#endif
