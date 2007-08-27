/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4env/include/ARCH-x86/env.h
 * \brief  L4 Environment public interface
 * \ingroup env
 *
 * \date   08/18/2000
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2007
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2 as
 * published by the Free Software Foundation (see the file COPYING).
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For different licensing schemes please contact
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/
#ifndef _L4_ENV_ENV_H
#define _L4_ENV_ENV_H

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>
#include <l4/env/system.h>
#include <l4/dm_generic/types.h>

/*****************************************************************************
 *** environment info page
 *****************************************************************************/

#define L4ENV_MAXSECT	64		///< max # of loadable sections
#define L4ENV_MAXPATH	256		///< max length of pathname

/**
 * Defines some L4 kernel information - some of them (arch, data, arch_class)
 * needed for loading an ELF binary.
 * \ingroup env
 */
typedef struct
{
  l4_uint32_t		major_id;	///< L4 major version id
  l4_uint32_t		minor_id;	///< L4 minor version id
  l4_uint8_t		arch_data;	///< see ELF data enc.: little/big end.
  l4_uint8_t		arch_class;	///< see ELF class enc.: 32/64 bit
  l4_uint16_t		arch;		///< use values here defined in elf.h
  l4_uint32_t		flags;		///< L4 kernel features
} l4env_version_info_t;

/* flags for the type field */

/** section is readable */
#define L4_DSTYPE_READ		0x0001

/** section is writable */
#define L4_DSTYPE_WRITE		0x0002

/** section is executable */
#define L4_DSTYPE_EXECUTE	0x0004

/** section must be relocated, that is the start address of the section
 * has to be defined. This flag is set for dynamic sections. */
#define L4_DSTYPE_RELOCME	0x0008

/** section has to be linked */
#define L4_DSTYPE_LINKME	0x0010

/** section has to be paged by an external pager (old-style applications)
 * or by the region manager (new-style applications) */
#define L4_DSTYPE_PAGEME	0x0020

/** reserve space for section in vm_area of application */
#define L4_DSTYPE_RESERVEME	0x0040

/** if this flag is set the section is shared to the target address space,
 * else the section is direct used. */
#define L4_DSTYPE_SHARE		0x0080

/** first section of an ELF object */
#define L4_DSTYPE_OBJ_BEGIN	0x0100

/** last section of an ELF object */
#define L4_DSTYPE_OBJ_END	0x0200

/** there was an error while this section was dynamically linked,
 * e.g. a symbol reference could not be resolved */
#define L4_DSTYPE_ERRLINK	0x0400

/** this section is needed for startup */
#define L4_DSTYPE_STARTUP	0x0800

/** this section is owned by app */
#define L4_DSTYPE_APP_IS_OWNER	0x1000

/** this section is owned by l4exec */
#define L4_DSTYPE_EXEC_IS_OWNER	0x2000

/** value of l4env_infopage->magic */
#define L4ENV_INFOPAGE_MAGIC 0x7634456e

/**
 * Section Info
 * \ingroup env
 */
typedef struct
{
  unsigned		id:16;		///< unique section id
  unsigned		type:16;	///< type info */
} l4exec_info_t;

/**
 * Defines a section of an executable file (binary or shared library)
 * \ingroup env
 */
typedef struct
{
  l4_addr_t		addr;		///< virtual address of the first byte
  l4_size_t		size;		///< size of the section
  l4dm_dataspace_t	ds;		///< underlaying dataspace
  l4exec_info_t	info;		        ///< id/type of section
} l4exec_section_t;

/**
 * Defines loader configuration information useful for programs.
 * \ingroup env
 */
typedef struct
{
  int		has_x86_vga : 1;	///< program has access to VGA memory
  int		has_x86_bios : 1;	///< program has access to BIOS memory
  int		hash_dyn_libs : 1;	///< integrity measurements for loaded libs
} l4env_loader_info_t;

/**
 * The environment info page - should consider 64-bit architectures too.
 * \ingroup env
 */
typedef struct
{
  /* system information provided by the loader */
  l4env_system_info_t   sys_info;		///< system info (proz, mem)
  l4env_version_info_t  ver_info;		///< kernel info (ver, arch)
  l4_uint32_t		num_threads;		///< # of threads used by task
  l4_uint32_t		stack_size;		///< size of stack
  l4_addr_t             vm_low;                 ///< virtual memory start addr
  l4_addr_t             vm_high;                ///< virtual memory end address

  l4_threadid_t	        names_id;		///< root name server
  l4_threadid_t	        memserv_id;		///< default memory server
  l4_threadid_t	        taskserv_id;		///< default task server
  l4_threadid_t	        fprov_id;		///< file provider (tftp...)
  l4_threadid_t	        loader_id;		///< loader
  l4_threadid_t		parent_id;		///< parent

  /* these entries are provided by the loader to define which dataspace
   * manager should be used for getting memory for a specific section */
  l4_threadid_t	        image_dm_id;		///< dm for file image
  l4_threadid_t	        text_dm_id;		///< dm for text segment
  l4_threadid_t	        data_dm_id;		///< dm for data segment
  l4_threadid_t	        stack_dm_id;		///< dm for stack segment
  l4_addr_t		entry_1st;		///< program entry (libloader)
  l4_addr_t		entry_2nd;		///< program entry (libl4env)

  /* these entries are filled by the execution layer server */
  int			section_num;		///< # of program sections
  l4exec_section_t	section[L4ENV_MAXSECT];	///< program section descs

  /* relocating addresses for libraries which are need fixed addresses
   * (libloader and libl4rm), because the l4rm can't page itself before
   * it's pager thread is started. */
  l4_addr_t		addr_libloader;		///< reloc address

  l4_addr_t		stack_low;		///< low bound of thread stack
  l4_addr_t		stack_high;		///< high bound of thread stack

  /* default path for loading binary objects if no path is given */
  char		        binpath[L4ENV_MAXPATH];	///< default bin path
  char		        libpath[L4ENV_MAXPATH];	///< default lib path

  l4_addr_t		addr_mb_info;		///< pointer to mb_info

  /* the following fields are used by the ld-l4.s.so interpreter */
  l4_addr_t		interp;			///< pointer to interpreter
  l4_addr_t		phdr;			///< pointer to program headers
  l4_uint32_t		phnum;			///< number of program headers
  int			num_init_fn;
  l4_addr_t		init_fn[64];		///< dynamic[DT_INIT] of shlibs

  l4env_loader_info_t	loader_info;		///< loader info for app

  l4_uint32_t		magic;			///< must be 0x7634456e
} l4env_infopage_t;


/*****************************************************************************
 *** request environment configuration
 *****************************************************************************/

/* keys */
#define L4ENV_MEMORY_SERVER         0x00000001   ///< default dataspace manager
#define L4ENV_TASK_SERVER           0x00000002   ///< default task server
#define L4ENV_NAME_SERVER           0x00000003   ///< name server
#define L4ENV_FPROV_SERVER          0x00000004   ///< default file provider
#define L4ENV_SIGMA0                0x00000005   ///< Sigma0

#define L4ENV_MAX_THREADS           0x00000010   ///< maxc. number of threads
#define L4ENV_DEFAULT_STACK_SIZE    0x00000011   ///< default stack size
#define L4ENV_MAX_STACK_SIZE        0x00000012   ///< max. stack size
#define L4ENV_DEFAULT_PRIO          0x00000013   ///< default thread priority
#define L4ENV_VM_LOW                0x00000014   ///< default VM low address
#define L4ENV_VM_HIGH               0x00000015   ///< default VM high address

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief  Request id of environment service.
 * \ingroup env
 *
 * \param  key           service key (see l4/env/env.h)
 * \retval service       service id
 *
 * \return 0 on success (\a service contains the requested service id),
 *         error code otherwise:
 *         - -L4_EINVAL   invlid key
 *         - -L4_ENODATA  value not set
 */
/*****************************************************************************/
int
l4env_request_service(l4_uint32_t key,
		      l4_threadid_t * service);

/*****************************************************************************/
/**
 * \brief  Request configuration dword.
 * \ingroup env
 *
 * \param  key           config key
 * \retval cfg           configuration value
 *
 * \return 0 on success (\a cfg contains the confguration value),
 *         error code otherwise:
 *         - -L4_EINVAL   invalid key
 *         - -L4_ENODATA  value not set
 */
/*****************************************************************************/
int
l4env_request_config_u32(l4_uint32_t key,
			 l4_addr_t * cfg);

/*****************************************************************************/
/**
 * \brief  Request configuration string.
 * \ingroup env
 *
 * \param  key           config key
 * \param  str           destination string buffer
 * \param  max_len       length of destination buffer
 *
 * \return 0 on succes (\a str contains the requested configuration string),
 *         error code otherwise:
 *         - -L4_EINVAL   invalid key
 *         - -L4_ENODATA  value not set
 */
/*****************************************************************************/
int
l4env_request_config_string(l4_uint32_t key,
			    char * str,
			    int max_len);

/*****************************************************************************/
/**
 * \brief  Return pointer to L4 environment page
 * \ingroup env
 *
 * \return Pointer to L4 environment page, NULL if no environment page
 *         available (i.e, the application was not started by our loader)
 */
/*****************************************************************************/
l4env_infopage_t *
l4env_get_infopage(void);

/*****************************************************************************/
/**
 * \brief  Set sigma0 Id
 * \ingroup env
 *
 * \param  id            Sigma0 id
 *
 * Set sigma0 id (used by startup code).
 */
/*****************************************************************************/
void
l4env_set_sigma0_id(l4_threadid_t id);

/*****************************************************************************/
/**
 * \brief  Set default dataspace manager
 * \ingroup env
 *
 * \param  id            Dataspace manager id
 */
/*****************************************************************************/
void
l4env_set_default_dsm(l4_threadid_t id);

/*****************************************************************************/
/**
 * \brief  Return default dataspace manager id
 *
 * \return Dataspace manager id, L4_INVALID_ID if no dataspace manager found
 */
/*****************************************************************************/
l4_threadid_t
l4env_get_default_dsm(void);

/*****************************************************************************/
/**
 * \brief  Return parent id
 *
 * \return parent ID or L4_NIL_ID
 */
/*****************************************************************************/
l4_threadid_t
l4env_get_parent(void);

__END_DECLS;

#endif /* _L4_ENV_ENV_H */

