/**
 * \file
 */

#include <l4/util/mb_info.h>
#include "startup.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_kip_v2(void *_l4i, boot_info_t *bi, l4util_mb_info_t *mbi);
void init_kip_v4(void *_l4i, boot_info_t *bi, l4util_mb_info_t *mbi);
void init_kip_arm(void *kip, boot_info_t *bi, l4util_mb_info_t *mbi);

#ifdef __cplusplus
}
#endif

