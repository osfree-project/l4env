#ifndef DM_H
#define DM_H

void       dm_start(void);
l4_int32_t dm_open(const char* fname, l4_uint32_t flags, l4_threadid_t dm_id,
		   l4_threadid_t client, l4dm_dataspace_t *ds,
		   l4_size_t *ds_size);

#endif
