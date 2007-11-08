#include <l4/log/l4log.h>
#include <l4/util/macros.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/reboot.h>
#include <linux/fs.h>

NORET_TYPE void complete_and_exit(struct completion *arg0, long arg1)
{
	Panic("NYI");

	for (;;) ;
}

void fastcall complete(struct completion *x)
{
	Panic("NYI");
}

void fastcall wait_for_completion(struct completion *x)
{
	Panic("NYI");
}

//int schedule_task(struct tq_struct *task)
//{
//	int ret;
//	need_keventd(__FUNCTION__);
//	ret = queue_task(task, &tq_context);
//	wake_up(&context_task_wq);
//	return ret;
//
//	PANIC("NYI");
//	return -123;
//}

int register_chrdev(unsigned int arg0, const char *arg1,
                    struct file_operations *arg2)
{
	LOG_Enter("%s", arg1);
	return 0;
}

int unregister_chrdev(unsigned int arg0, const char *arg1)
{
	LOG_Enter("%s", arg1);
	return 0;
}

/* XXX */
int kill_proc(pid_t arg0, int arg1, int arg2)
{
	Panic("NYI");
	return 0;
}
