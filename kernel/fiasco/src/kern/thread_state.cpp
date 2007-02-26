INTERFACE:

// When changing these constants, see also entry.S: RESET_THREAD_STATE.
enum Thread_state
{
  Thread_invalid = 0,		// tcb unallocated
  Thread_running = 0x1,		// can be scheduled
  Thread_waiting = 0x2,		// waiting for any message
  Thread_receiving = 0x4,	// waiting for message from specified sender
  Thread_polling = 0x8,		// waiting to send message to specified rcvr
  Thread_ipc_in_progress = 0x10,// an IPC operation has not been finished
  Thread_send_in_progress = 0x20,// an IPC send operation has not been finished
  Thread_busy = 0x40,		// an IPC rcv handshake has not been finished
  Thread_cancel = 0x100,	// state has been changed -- cancel activity
  Thread_dead = 0x200,		// tcb allocated, but inactive (not in any q)
  Thread_polling_long = 0x400,	// sender waiting for rcvr to page in data
  Thread_busy_long = 0x800,	// receiver going to sleep in long IPC
  Thread_rcvlong_in_progress = 0x4000,	// receiver in long IPC
  Thread_fpu_owner = 0x10000,	// currently owns the fpu

  // constants for convenience
  Thread_ipc_sending_mask = Thread_send_in_progress
    | Thread_polling | Thread_polling_long,
  Thread_ipc_receiving_mask = Thread_waiting | Thread_receiving 
    | Thread_busy |  Thread_rcvlong_in_progress | Thread_busy_long,
  Thread_ipc_mask = Thread_ipc_in_progress
    | Thread_ipc_sending_mask | Thread_ipc_receiving_mask,
    
};
