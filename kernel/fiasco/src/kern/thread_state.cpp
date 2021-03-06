INTERFACE:

// When changing these constants, change them also in shared/shortcut.h!
enum Thread_state
{
  Thread_invalid          = 0,	  // tcb unallocated
  Thread_ready            = 0x1,  // can be scheduled
  Thread_utcb_ip_sp       = 0x2,  // needs to reload user ip/sp from utcb
  Thread_receiving        = 0x4,  // waiting for message from specified sender
  Thread_polling          = 0x8,  // waiting to send message to specified rcvr
  Thread_ipc_in_progress  = 0x10, // an IPC operation has not been finished
  Thread_send_in_progress = 0x20, // an IPC send operation hasn't been finished
  Thread_busy             = 0x40, // an IPC rcv handshake has not been finished
  // the ipc handshake bits needs to be in the first byte, 
  // the shortcut depends on it

  Thread_cancel           = 0x100,// state has been changed -- cancel activity
  Thread_dead             = 0x200,// tcb allocated, but inactive (not in any q)
  Thread_polling_long     = 0x400,// sender waiting for rcvr to page in data
  Thread_busy_long        = 0x800,  // receiver going to sleep in long IPC
  Thread_rcvlong_in_progress = 0x1000,	// receiver in long IPC
  Thread_delayed_deadline = 0x2000, // delayed until periodic deadline
  Thread_delayed_ipc      = 0x4000, // delayed until periodic ipc event 
  Thread_fpu_owner        = 0x8000, // currently owns the fpu
  Thread_alien            = 0x10000, // Thread is an alien, is not allowed 
                                     // to do system calls
  Thread_dis_alien        = 0x20000, // Thread is an alien, however the next
                                     // system call is allowed
  Thread_in_exception     = 0x40000, // Thread has sent an exception but still
                                     // got no reply
  Thread_transfer_in_progress        = 0x80000, // ipc transfer

  // constants for convenience
  Thread_ipc_sending_mask    = Thread_send_in_progress |
                               Thread_polling |
                               Thread_polling_long,

  Thread_ipc_receiving_mask  = Thread_receiving |
                               Thread_busy |
                               Thread_rcvlong_in_progress |
                               Thread_transfer_in_progress|
                               Thread_busy_long,

  Thread_ipc_mask            = Thread_ipc_in_progress |
                               Thread_ipc_sending_mask |
                               Thread_ipc_receiving_mask,
};
