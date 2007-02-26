#ifndef EXEC_H
#define EXEC_H

#include "types.h"

#define EXEC_SECTYPE_READ		((exec_sectype_t)0x000001)
#define EXEC_SECTYPE_WRITE		((exec_sectype_t)0x000002)
#define EXEC_SECTYPE_EXECUTE		((exec_sectype_t)0x000004)
#define EXEC_SECTYPE_PROT_MASK		((exec_sectype_t)0x000007)
#define EXEC_SECTYPE_ALLOC		((exec_sectype_t)0x000100)
#define EXEC_SECTYPE_LOAD		((exec_sectype_t)0x000200)
#define EXEC_SECTYPE_DEBUG		((exec_sectype_t)0x010000)
#define EXEC_SECTYPE_AOUT_SYMTAB	((exec_sectype_t)0x020000)
#define EXEC_SECTYPE_AOUT_STRTAB	((exec_sectype_t)0x040000)

/*
 * Error codes
 * XXX these values are a holdover from Mach 3, and probably should be changed.
 */

#define	EX_NOT_EXECUTABLE	6000	/* not a recognized executable format */
#define	EX_WRONG_ARCH		6001	/* valid executable, but wrong arch. */
#define EX_CORRUPT		6002	/* recognized executable, but mangled */
#define EX_BAD_LAYOUT		6003	/* something wrong with the memory or file image layout */

/* file class or capacity - page 4-8 */

#define ELFCLASSNONE	0
#define ELFCLASS32	1
#define ELFCLASS64	2

#ifdef ARCH_x86
/* Architecture identification parameters for x86 architecture.  */
#define MY_EI_DATA	ELFDATA2LSB
#define MY_E_MACHINE	EM_386
#endif

#ifdef ARCH_arm
/* Architecture identification parameters for x86 architecture.  */
#define MY_EI_DATA	ELFDATA2LSB
#define MY_E_MACHINE	EM_ARM
#endif

/* XXX from flux XXX */
typedef int exec_sectype_t;


typedef enum
{
	EXEC_ELF	= 1,
	EXEC_AOUT	= 2,
} exec_format_t;

typedef struct exec_info
{
	/* Format of executable loaded - see above.  */
//	exec_format_t format;

	/* Program entrypoint.  */
	l4_addr_t entry;

	/* Initial data pointer - only some architectures use this.  */
//	l4_addr_t init_dp;

	/* (ELF) Address of interpreter string for loading shared libraries,
	   null if none.  */
//	l4_addr_t interp;

} exec_info_t;

typedef struct exec_task
{
  void *mod_start;
  unsigned mod_no;
  unsigned task_no;
  l4_addr_t begin; /* program begin */
  l4_addr_t end; /* program end */
} exec_task_t;

typedef int exec_read_func_t(void *handle, l4_addr_t file_ofs,
                             void *buf, l4_size_t size,
                             l4_size_t *out_actual);

typedef int exec_read_exec_func_t(void *handle,
                                  l4_addr_t file_ofs, l4_size_t file_size,
                                  l4_addr_t mem_addr, l4_size_t mem_size,
                                  exec_sectype_t section_type);

int exec_load_elf(exec_read_func_t *read,
                  exec_read_exec_func_t *read_exec,
                  void *handle, exec_info_t *out_info);

#endif
