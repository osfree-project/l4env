#ifndef MODULE_H
#define MODULE_H

#include <stddef.h>
#include <l4/util/mb_info.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

void print_module_name(const char *name, const char *alt_name);

EXTERN_C_END

#endif
