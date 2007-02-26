#ifndef __DIETLIBC_LIB_SERVER_NAME_SERVER_INCLUDE_NAME_SPACE_PROVIDER_H_
#define __DIETLIBC_LIB_SERVER_NAME_SERVER_INCLUDE_NAME_SPACE_PROVIDER_H_

int register_volume(l4_threadid_t server,
                    volume_id_t volume_id);

int unregister_volume(l4_threadid_t server,
                      volume_id_t volume_id);

#endif
