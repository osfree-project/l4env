/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux26/lib/src/init_task.c
 * \brief  Task initialisation
 *
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/init_task.h>
#include <linux/fs.h>
#include <linux/personality.h>

#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/desc.h>

static void default_handler(int,struct pt_regs*);

struct fs_struct init_fs = INIT_FS;
struct files_struct init_files = INIT_FILES;
struct signal_struct init_signals = INIT_SIGNALS(init_signals);
struct sighand_struct init_sighand = INIT_SIGHAND(init_sighand);
//struct mm_struct init_mm = INIT_MM(init_mm);
struct mm_struct init_mm;

static u_long ident_map[32] = {
    0,1,2,3,4,5,6,7,
    8,9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,
    24,25,26,27,28,29,30,31
};

struct exec_domain default_exec_domain = {
   .name	= "Linux",
   .handler	= default_handler,
   .pers_low	= 0,
   .pers_high	= 0,
   .signal_map	= ident_map,
   .signal_invmap = ident_map,
};

static void default_handler(int segment, struct pt_regs *regp)
{
}

long do_no_restart_syscall(struct restart_block *param)
{
    return -EINTR;
}
/*
 * Initial thread structure.
 *
 * We need to make sure that this is 8192-byte aligned due to the
 * way process stacks are handled. This is done by having a special
 * "init_task" linker map entry..
 */
union thread_union init_thread_union 
	__attribute__((__section__(".data.init_task"))) =
		{ INIT_THREAD_INFO(init_task) };

/*
 * Initial task structure.
 *
 * All other task structs will be allocated on slabs in fork.c
 */
struct task_struct init_task = INIT_TASK(init_task);

/*
 * per-CPU TSS segments. Threads are completely 'soft' on Linux,
 * no more per-task TSS's. The TSS size is kept cacheline-aligned
 * so they are allowed to end up in the .data.cacheline_aligned
 * section. Since TSS's are completely CPU-local, we want them
 * on exact cacheline boundaries, to eliminate cacheline ping-pong.
 */ 
struct tss_struct init_tss[NR_CPUS] __cacheline_aligned = { [0 ... NR_CPUS-1] = INIT_TSS };

