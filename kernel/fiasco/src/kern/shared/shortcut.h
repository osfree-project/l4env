#ifndef SHORTCUT_H
#define SHORTCUT_H

#include "globalconfig.h"

// thread_state consts
#define Thread_ready			0x1
#define Thread_utcb_ip_sp		0x2
#define Thread_receiving		0x4
#define Thread_polling			0x8
#define	Thread_ipc_in_progress		0x10
#define Thread_send_in_progress		0x20
#define Thread_busy			0x40
#define Thread_lipc_ready		0x80
#define Thread_cancel			0x100
#define Thread_dead			0x200
#define Thread_polling_long		0x400
#define Thread_busy_long		0x800
#define Thread_rcvlong_in_progress	0x1000
#define Thread_delayed_deadline		0x2000
#define Thread_delayed_ipc		0x4000
#define Thread_fpu_owner		0x8000
#define Thread_alien			0x10000
#define Thread_dis_alien		0x20000

#define Thread_ipc_sending_mask        (Thread_send_in_progress		| \
					Thread_polling			| \
					Thread_polling_long)
#define Thread_ipc_receiving_mask      (Thread_receiving		| \
					Thread_busy			| \
   					Thread_rcvlong_in_progress	| \
   					Thread_busy_long)
#define Thread_ipc_mask                (Thread_ipc_in_progress		| \
					Thread_ipc_sending_mask		| \
					Thread_ipc_receiving_mask)

// stackframe structure
#define REG_ECX
#define REG_EDX	(1*4)
#define REG_ESI	(2*4)
#define REG_EDI	(3*4)
#define REG_EBX	(4*4)
#define REG_EBP	(5*4)
#define REG_EAX	(6*4)
#define REG_EIP (7*4)
#define REG_CS	(8*4)
#define REG_EFL	(9*4)
#define REG_ESP	(10*4)
#define REG_SS	(11*4)

#ifdef CONFIG_ABI_X0
#  define RETURN_DOPE 0x6000 // three dwords
#  define TCB_ADDRESS_MASK 0x01fff800
#else
#  define RETURN_DOPE 0x4000 // two dwords
#  define TCB_ADDRESS_MASK 0x1ffff800
#endif


#if defined(CONFIG_JDB) && defined(CONFIG_JDB_ACCOUNTING)

#define CNT_CONTEXT_SWITCH	incl KSEG (VAL__MEM_LAYOUT__TBUF_STATUS_PAGE+ \
				    OFS__TBUF_STATUS__KERNCNTS)
#define CNT_ADDR_SPACE_SWITCH	incl KSEG (VAL__MEM_LAYOUT__TBUF_STATUS_PAGE+ \
				    OFS__TBUF_STATUS__KERNCNTS + 4)
#define CNT_SHORTCUT_FAILED	incl KSEG (VAL__MEM_LAYOUT__TBUF_STATUS_PAGE+ \
				    OFS__TBUF_STATUS__KERNCNTS + 8)
#define CNT_SHORTCUT_SUCCESS	incl KSEG (VAL__MEM_LAYOUT__TBUF_STATUS_PAGE+ \
				    OFS__TBUF_STATUS__KERNCNTS + 12)
#define CNT_IOBMAP_TLB_FLUSH	incl KSEG (VAL__MEM_LAYOUT__TBUF_STATUS_PAGE+ \
				    OFS__TBUF_STATUS__KERNCNTS + 40)

#else

#define CNT_CONTEXT_SWITCH
#define CNT_ADDR_SPACE_SWITCH
#define CNT_SHORTCUT_FAILED
#define CNT_SHORTCUT_SUCCESS
#define CNT_IOBMAP_TLB_FLUSH

#endif

#endif
