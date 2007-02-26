#ifndef PATCH_H
#define PATCH_H

void        patch_module(const char **str, l4util_mb_info_t *mbi);
void        args_module(const char **str, l4util_mb_info_t *mbi);
const char *get_args_module(int i);

#endif
