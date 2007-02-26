/*
 * \brief   Thread table implementation
 * \date    2004-10-26
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 */

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/sys/types.h>
#include <l4/log/l4log.h>

/*** LOCAL INCLUDES ***/
#include "local.h"

#define MAX_THREADS 128

static thread_entry_t thread_tbl[MAX_THREADS];

int tt_get_entry_id(int fd, l4_threadid_t owner_tid)
{
	int i;

	for (i = 0; i < MAX_THREADS; i++)
	{
		
		if ((thread_tbl[i].fd == fd)
			 && (l4_thread_equal(thread_tbl[i].owner_tid, owner_tid)))
			break;
	}

	if (i >= MAX_THREADS)
		return -1;
	else
		return i;
}

thread_entry_t tt_get_entry(int fd, l4_threadid_t owner_tid)
{
	thread_entry_t tt;
	int i;

	for (i = 0; i < MAX_THREADS; i++)
	{
		
		if ((thread_tbl[i].fd == fd)
			 && (l4_thread_equal(thread_tbl[i].owner_tid, owner_tid)))
			break;
	}

	if (i >= MAX_THREADS)
{
		tt.fd = -1;
		tt.worker_tid = L4_INVALID_ID;
		tt.owner_tid = L4_INVALID_ID;
		tt.private_data = NULL;
		return tt;
	}

	return thread_tbl[i];
}

int tt_get_next_free_entry(void)
{
	int i;

	for (i = 0; i < MAX_THREADS && (thread_tbl[i].fd >= 0); i++);

	if (i >= MAX_THREADS)
		return -1;
	else
		return i;
}

void tt_free_entry(int fd, l4_threadid_t owner_tid)
{
	int i;

	for (i = 0; i < MAX_THREADS; i++)
	{
		if ((thread_tbl[i].fd == fd)
			 && (l4_thread_equal(thread_tbl[i].owner_tid, owner_tid)))
			break;
	}

	if (i >= MAX_THREADS)
		return;

	thread_tbl[i].fd = -1;
	thread_tbl[i].worker_tid = L4_INVALID_ID;
	thread_tbl[i].owner_tid = L4_INVALID_ID;
	thread_tbl[i].private_data = NULL;
}

void tt_fill_entry(int id, thread_entry_t entry)
{

	if ((id < 0) || (id >= MAX_THREADS))
		return;

	thread_tbl[id].fd = entry.fd;
	thread_tbl[id].worker_tid = entry.worker_tid;
	thread_tbl[id].owner_tid = entry.owner_tid;
}

void tt_init(void)
{
	int i;

	for (i = 0; i < MAX_THREADS; i++)
	{
		thread_tbl[i].fd = -1;
		thread_tbl[i].worker_tid = L4_INVALID_ID;
		thread_tbl[i].owner_tid = L4_INVALID_ID;
		thread_tbl[i].private_data = NULL;
	}
}

void * tt_entry_get_private_data(thread_entry_t *entry)
{
	return entry->private_data;
}

void tt_entry_set_private_data(thread_entry_t *entry, void *data)
{
	entry->private_data = data;
}
