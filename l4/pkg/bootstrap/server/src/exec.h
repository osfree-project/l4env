#ifndef EXEC_H
#define EXEC_H

#include "types.h"
#include <l4/sys/compiler.h>
#include <l4/util/mb_info.h>

typedef int exec_sectype_t;

#define EXEC_SECTYPE_READ		((exec_sectype_t)0x000001)
#define EXEC_SECTYPE_WRITE		((exec_sectype_t)0x000002)
#define EXEC_SECTYPE_EXECUTE		((exec_sectype_t)0x000004)
#define EXEC_SECTYPE_ALLOC		((exec_sectype_t)0x000100)
#define EXEC_SECTYPE_LOAD		((exec_sectype_t)0x000200)


typedef struct
{
  void *mod_start;
  l4util_mb_mod_t *mod;
  unsigned type;
  l4_addr_t begin;	/* program begin */
  l4_addr_t end;	/* program end */
} exec_task_t;


typedef int exec_handler_func_t(void *handle,
				  l4_addr_t file_ofs, l4_size_t file_size,
				  l4_addr_t mem_addr, l4_addr_t v_addr,
				  l4_size_t mem_size,
				  exec_sectype_t section_type);

EXTERN_C_BEGIN

int exec_load_elf(exec_handler_func_t *handler_exec,
		  void *handle, const char **error_msg, l4_addr_t *entry);

EXTERN_C_END

#endif
