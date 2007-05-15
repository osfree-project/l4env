#ifndef INIT_KIP_H__
#define INIT_KIP_H__

#include <l4/util/mb_info.h>
#include "startup.h"

class Region_list;

void init_kip_v2(void *_l4i, boot_info_t *bi, l4util_mb_info_t *mbi,
    Region_list *ram, Region_list *regions);
void init_kip_v4(void *_l4i, boot_info_t *bi, l4util_mb_info_t *mbi,
    Region_list *ram, Region_list *regions);

#endif

