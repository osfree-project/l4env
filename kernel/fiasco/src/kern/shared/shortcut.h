#ifndef SHORTCUT_H
#define SHORTCUT_H

// thread_state consts
#define Thread_running			0x0001
#define Thread_waiting			0x0002
#define Thread_receiving		0x0004
#define Thread_polling			0x0008
#define	Thread_ipc_in_progress		0x0010
#define Thread_send_in_progress		0x0020
#define Thread_busy			0x0040
#define Thread_cancel			0x0100
#define Thread_dead			0x0200
#define Thread_polling_long		0x0400
#define Thread_busy_long		0x0800
#define Thread_rcvlong_in_progress	0x4000
#define Thread_fpu_owner		0x10000
#define Thread_ipc_sending_mask	\
  (Thread_send_in_progress | Thread_polling | Thread_polling_long)
#define Thread_ipc_receiving_mask \
  (Thread_waiting | Thread_receiving | Thread_busy | \
   Thread_rcvlong_in_progress | Thread_busy_long)
#define Thread_ipc_mask	\
  (Thread_ipc_in_progress | Thread_ipc_sending_mask | Thread_ipc_receiving_mask)

// some other consts
#define ipc_window0			0xee000000
#define ipc_window1			0xee800000
#define smas_version			0xef000000
#define smas_area			0xef400000
#define smas_start			0xeb000000
#define smas_end			0xee000000

// see kmem-ia32.cpp
#define smas_trampoline			(0xeac00000 + 0x3000)

#endif

