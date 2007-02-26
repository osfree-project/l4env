/* $Id$ */
/*****************************************************************************/
/**
 * \file   genric_blk/examples/linux/linux_emul.c
 * \brief  L4Linux block device stub, emulation stuff
 *
 * \date   02/25/2003
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* prevent definition of __module_kernel_version */
#define __NO_VERSION__

/* Linux includes */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/malloc.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/personality.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/string.h>

#include <l4linux/x86/ids.h>

#ifndef CONFIG_L4_THREADMANAGEMENT
#error The L4 Block Device Stub Driver requires the threadmanagement.
#error Enable the CONFIG_L4_THREADMANAGEMENT option!
#endif

#ifndef CONFIG_L4_EXTERNAL_IRQS
#error The L4 Block Device Stub Driver acts as an external IRQ source.
#error Enable the CONFIG_L4_EXTERNAL_IRQS option!
#endif

/*****************************************************************************
 *** Linux thread stack management
 *** Linux expects a certain stack layout (i.e. the L4 thread id on top of the 
 *** stack and a task struct at the end of stack which must contain reasonable 
 *** default values), taken from arch/l4/x86/kernel/init_task.c
 *****************************************************************************/

/* default values for the tast struct */
static struct vm_area_struct init_mmap = INIT_MMAP;
static struct fs_struct init_fs = INIT_FS;
static struct files_struct init_files = INIT_FILES;
static struct signal_struct init_signals = INIT_SIGNALS;
static struct mm_struct init_mm = INIT_MM;

static struct exec_domain default_exec_domain = {
  "Linux",   /* name */
  NULL,      /* lcall7 causes a seg fault. */
  0, 0xff,   /* All personalities. */
  NULL,      /* Identity map signals. */
  NULL,      /*  - both ways. */
  NULL,      /* No usage counter. */
  NULL       /* Nothing after this in the list. */
};

static struct task_struct *task[NR_TASKS];
static struct task_struct l4blk_task_init = INIT_TASK;

void it_real_fn(unsigned long __data){}

/* kernel stack size, in 'order' (1 << (n + LOG2_PAGESIZE)) */
#define L4LINUX_KERNEL_STACK_ORDER  1    /* 8192 bytes */
#define L4LINUX_KERNEL_STACK_SIZE \
  (1 << (L4LINUX_KERNEL_STACK_ORDER + L4_LOG2_PAGESIZE))

/*****************************************************************************/
/**
 * \brief  Allocate stack for L4 thread to be used in the Linux kernel
 * 
 * \param  thread        Thread id of the new thread, it will be put on 
 *                       top of the stack
 *	
 * \return Initial stack pointer of the new stack, NULL if allocation failed
 */
/*****************************************************************************/ 
void *
l4blk_allocate_stack(l4_threadid_t thread)
{
  void * tmp, * sp;

  /* allocate stack */
  tmp = (void *)__get_free_pages(GFP_KERNEL,L4LINUX_KERNEL_STACK_ORDER);
  if (tmp == NULL)
    return NULL;
  
  /* put dummy task struct to the end of the stack */
  memcpy(tmp,&l4blk_task_init,sizeof(struct task_struct));

  /* put thread id on top of the stack */
  sp = tmp + L4LINUX_KERNEL_STACK_SIZE - 4;
  put_l4_id_to_stack((unsigned)sp, thread);
  sp -= sizeof(l4_threadid_t);

  /* done */
  return sp;
}

/*****************************************************************************/
/**
 * \brief  Release stack
 * 
 * \param  sp            Stack pointer
 */
/*****************************************************************************/ 
void
l4blk_release_stack(void * sp)
{
  /* we cannot release the stack, there is a race in destroy_thread which 
   * might cause further stack accesses */
}
