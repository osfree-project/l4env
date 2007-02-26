/*!
 * \author Jork Loeser <jork.loeser@os.inf.tu-dresden.de>
 * locking with the rtnl semaphore
 * Functions originally in net/core/rtnetlink.c
 */

#include <l4/semaphore/semaphore.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/rtnetlink.h>
#include <linuxemul.h>

l4semaphore_t rtnl_sem_ore = L4SEMAPHORE_UNLOCKED_INITIALIZER;

/* These functions seem to lock the device list, which is used in the
   routing code. While we have no routing code, we still insert
   devices. There we use the original code which needs the locking. */

/* originally uses rtnl_shlock, which is defined as down(&rtnl_sem) */
void rtnl_lock(void){
    l4semaphore_down(&rtnl_sem_ore);
}

/* originally uses rtnl_shunlock, which is defined as:
   up(&rtnl_sem);
   if (rtnl && rtnl->receive_queue.qlen) rtnl->data_ready(rtnl, 0);

   However, we should not have anybody using this queue.
*/
void rtnl_unlock(void){
    l4semaphore_up(&rtnl_sem_ore);
}

void __rta_fill(struct sk_buff *skb, int attrtype, int attrlen, const void *data)
{
	struct rtattr *rta;
	int size = RTA_LENGTH(attrlen);

	rta = (struct rtattr*)skb_put(skb, RTA_ALIGN(size));
	rta->rta_type = attrtype;
	rta->rta_len = size;
	memcpy(RTA_DATA(rta), data, attrlen);
}

