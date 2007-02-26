/*!
 * \file	loader/server/src/cfg.h
 * \brief	config script stuff
 *
 * \date	2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __CFG_H_
#define __CFG_H_

#include <l4/sys/types.h>
#include <l4/dm_generic/types.h>
#include <l4/env/env.h>

#define CFG_MAX_MODULE		16	/* max # of modules per task */
#define CFG_MAX_MEM		4	/* max # of mem regions per task */
#define CFG_MAX_TASK		16	/* max # of tasks */

typedef struct
{
  const char       *fname;
  const char       *args;
  l4_addr_t        low;
  l4_addr_t        high;
} cfg_module_t;

typedef struct
{
  l4_addr_t        low;
  l4_addr_t        high;
  l4_size_t        size;
  l4_uint16_t      flags;
#define CFG_M_DMA_ABLE		0x00000001
#define CFG_M_CONTIGUOUS	0x00000002
#define CFG_M_DIRECT_MAPPED	0x00000004
#define CFG_M_NOSUPERPAGES	0x00000008
  l4_uint16_t      pool;
} cfg_mem_t;

typedef struct cfg_cap_t
{
  struct cfg_cap_t *next;
  char             *dest;
  char             type;
#define CAP_TYPE_ALLOW          1
#define CAP_TYPE_DENY           2
} cfg_cap_t;

typedef struct
{
  cfg_module_t     task;                   /**< fname, parameters */
  cfg_module_t     module[CFG_MAX_MODULE]; /**< fname, parameters of modules */
  cfg_module_t     *next_module;           /**< next free module */
  cfg_mem_t        mem[CFG_MAX_MEM];       /**< memory blocks */
  cfg_mem_t        *next_mem;              /**< next free memory block */
  char             *iobitmap;              /**< I/O permission bitmap */
  unsigned int     prio;                   /**< priority of thread x.0 */
  unsigned int     mcp;                    /**< mcp of thread x.0 */
  l4_threadid_t    fprov_id;               /**< file provider to read modules
					        and shared libraries from */
  l4_threadid_t    dsm_id;                 /**< dataspace manager for program
					        sections and modules */
  l4_threadid_t    caphandler;             /**< capability fault handler */
  cfg_cap_t        *caplist;               /**< list of capabilities */
  l4_addr_t        image;                  /**< attached binary image */
  l4_size_t        sz_image;               /**< size of binary image */
  l4dm_dataspace_t ds_image;               /**< dataspace of program image */
  l4_taskid_t      task_id;                /**< filled in after task startup */
  l4_uint32_t      flags;                  /**< see CFG_F_ constants */
#define CFG_F_TEMPLATE		0x00000001 /**< this is a template */
#define CFG_F_MEMDUMP		0x00000002 /**< dump memory */
#define CFG_F_DIRECT_MAPPED	0x00000004 /**< sections are mapped 1:1 */
#define CFG_F_REBOOT_ABLE	0x00000008 /**< application may reboot */
#define CFG_F_NO_VGA		0x00000010 /**< deny access to vga */
#define CFG_F_SLEEP		0x00000020 /**< sleep before continue */
#define CFG_F_NO_SIGMA0		0x00000040 /**< dont page other regions */
#define CFG_F_ALLOW_CLI		0x00000080 /**< task may execute cli/sti */
#define CFG_F_SHOW_APP_AREAS	0x00000100 /**< show app areas before start */
#define CFG_F_STOP		0x00000200 /**< stop app just before start */
#define CFG_F_NOSUPERPAGES	0x00000400 /**< don't use superpages */
#define CFG_F_INTERPRETER	0x00000800 /**< interpret using libld-l4.s.so */
#define CFG_F_ALL_WRITABLE	0x00001000 /**< all sections writable */
#define CFG_F_L4ENV_BINARY	0x00002000 /**< l4env binary with infopage */
} cfg_task_t;

extern unsigned int cfg_verbose;
extern unsigned int cfg_fiasco_symbols;
extern unsigned int cfg_fiasco_lines;
extern char cfg_binpath[L4ENV_MAXPATH];
extern char cfg_libpath[L4ENV_MAXPATH];
extern char cfg_modpath[L4ENV_MAXPATH];

int  cfg_new_task(const char *fname, const char *args);
void cfg_new_task_template(void);
int  cfg_new_module(const char *fname, const char *args,
		    l4_addr_t low, l4_addr_t high);
int  cfg_new_mem(l4_size_t size, l4_addr_t low, l4_addr_t high,
	         l4_umword_t flags);
int  cfg_new_ioport(int low, int high);
cfg_task_t** cfg_next_task(void);
int  cfg_task_prio(unsigned int prio);
int  cfg_task_mcp (unsigned int mcp);
int  cfg_task_flag(unsigned int flag);
int  cfg_task_fprov(const char *fname);
int  cfg_task_dsm(const char *fname);
int  cfg_task_caphandler(const char *fname);
int  cfg_task_ipc(const char *name, int type);

int cfg_lookup_name(const char *name, l4_threadid_t *id);
void cfg_setup_input(const char *cfg_buffer, int size);
void cfg_done(void);
int  cfg_parse(void);
int  cfg_parse_init(void);
int  cfg_job(unsigned int flag, unsigned int number);
int  load_config_script(const char *fname_and_arg, l4_threadid_t fprov_id,
			const l4dm_dataspace_t *cfg_ds, l4_addr_t cfg_addr, 
			l4_size_t cfg_size, l4_taskid_t owner,
			l4_uint32_t flags, int *is_binary,
			l4_taskid_t task_ids[]);
int  load_config_script_from_file(const char *fname_and_arg,
				  l4_threadid_t fprov_id, l4_taskid_t owner,
				  l4_uint32_t flags, l4_taskid_t task_ids[]);
int  cfg_init(void);

#endif
