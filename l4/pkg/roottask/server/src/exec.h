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


/* file class or capacity - page 4-8 */
#define ELFCLASSNONE	0
#define ELFCLASS32	1
#define ELFCLASS64	2

/* Architecture identification parameters.  */
#ifdef ARCH_x86
#define MY_EI_DATA	ELFDATA2LSB
#define MY_E_MACHINE	EM_386
#define MY_EI_CLASS	ELFCLASS32
#define MY_EHDR		Elf32_Ehdr
#define MY_PHDR		Elf32_Phdr
#define MY_SHDR		Elf32_Shdr
#define MY_SYM		Elf32_Sym
#else
#ifdef ARCH_amd64
#define MY_EI_DATA	ELFDATA2LSB
#define MY_E_MACHINE	EM_AMD64
#define MY_EI_CLASS	ELFCLASS64
#define MY_EHDR		Elf64_Ehdr
#define MY_PHDR		Elf64_Phdr
#define MY_SHDR		Elf64_Shdr
#define MY_SYM		Elf64_Sym
#else
#ifdef ARCH_arm
#define MY_EI_DATA	ELFDATA2LSB
#define MY_E_MACHINE	EM_ARM
#define MY_EI_CLASS	ELFCLASS32
#define MY_EHDR		Elf32_Ehdr
#define MY_PHDR		Elf32_Phdr
#define MY_SHDR		Elf32_Shdr
#define MY_SYM		Elf32_Sym
#else
#error Unsupported architecture!
#endif
#endif
#endif

typedef int exec_sectype_t;

typedef enum
{
  EXEC_ELF	= 1,
  EXEC_AOUT	= 2,
} exec_format_t;

typedef struct
{
  void *mod_start;
  unsigned mod_no;
  unsigned task_no;
  l4_addr_t begin;	/* program begin */
  l4_addr_t end;	/* program end */
} exec_task_t;

typedef int exec_read_func_t(void *handle, l4_addr_t file_ofs,
			     void *buf, l4_size_t size,
			     l4_size_t *out_actual);

typedef int exec_read_exec_func_t(void *handle,
				  l4_addr_t file_ofs, l4_size_t file_size,
				  l4_addr_t mem_addr, l4_size_t mem_size,
				  exec_sectype_t section_type);

int exec_load_elf(exec_read_func_t *read, exec_read_exec_func_t *read_exec,
		  void *handle, const char **error_msg, l4_addr_t *entry);

#endif
