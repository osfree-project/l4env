#ifndef __LOADER_CONFIG_H_
#define __LOADER_CONFIG_H_

#include <l4/sys/types.h>

#define CFG_MAX_MODULE		16	/* max # of modules per task */
#define CFG_MAX_MEM		4	/* max # of mem regions per task */
#define CFG_MAX_TASK		16	/* max # of tasks */

typedef struct
{
  const char *fname;
  const char *args;
} cfg_module_t;

typedef struct
{
  unsigned int low;
  unsigned int high;
  unsigned int size;
  unsigned int flags;
#define CFG_M_DMA_ABLE		0x00000001
#define CFG_M_CONTIGUOUS	0x00000002
#define CFG_M_DIRECT_MAPPED	0x00000004
} cfg_mem_t;

typedef struct
{
  cfg_module_t task;
  cfg_module_t module[CFG_MAX_MODULE];
  cfg_module_t *next_module;
  cfg_mem_t    mem[CFG_MAX_MEM];
  cfg_mem_t    *next_mem;
  unsigned int prio;
  unsigned int flags;
#define CFG_F_MEMDUMP		0x00000001	/* dump memory */
#define CFG_F_DIRECT_MAPPED	0x00000002	/* sections are mapped 1:1 */
#define CFG_F_REBOOT_ABLE	0x00000004	/* application may reboot */
#define CFG_F_NO_VGA		0x00000008	/* deny access to vga */
#define CFG_F_SLEEP		0x00000010	/* sleep before continue */
#define CFG_F_NO_SIGMA_NULL	0x00000020	/* dont page other regions */
} cfg_task_t;

extern unsigned int cfg_verbose;
extern unsigned int cfg_fiasco_symbols;
extern unsigned int cfg_fiasco_lines;
extern char cfg_binpath[];
extern char cfg_libpath[];
extern char cfg_modpath[];

int  cfg_new_module(const char *fname, const char *args);
int  cfg_new_mem(unsigned int size, unsigned int low, unsigned int high,
		 unsigned int flags);
int  cfg_new_task(const char *fname, const char *args, unsigned int flags);
cfg_task_t** cfg_next_task(void);
int  cfg_set_prio(unsigned int prio);

void cfg_setup_input(const char *cfg_buffer, int size);
int  cfg_parse(void);
int __attribute__ ((format(printf, 2, 3)))
  add_task_arg(cfg_task_t *ct, const char *format, ...);
int  cfg_init(void);
int  cfg_job(unsigned int flag, unsigned int number);
int  load_config_script(const char *fname, l4_threadid_t fprov_id);

#endif

