#ifndef MODULE_H
#define MODULE_H

#include <l4/util/mb_info.h>

extern inline const char*
get_module_name(l4util_mb_mod_t *mb_mod, char *default_name);

/* return module name */
extern inline const char*
get_module_name(l4util_mb_mod_t *mb_mod, char *default_name)
{
  return (mb_mod->cmdline) ? (char *)mb_mod->cmdline : default_name;
}

void print_module_name(const char *name, int length, int max_length);

#endif
