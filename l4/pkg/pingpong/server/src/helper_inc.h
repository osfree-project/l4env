
/** shake hands with tid */
void
PREFIX(call)(l4_threadid_t tid)
{
  l4_msgdope_t result;
  l4_umword_t dummy;

  l4_ipc_call(tid,
		   L4_IPC_SHORT_MSG,1,1,
		   L4_IPC_SHORT_MSG,&dummy,&dummy,
		   L4_IPC_NEVER,&result);
}

/** send short message to tid */
void
PREFIX(send)(l4_threadid_t tid)
{
  l4_msgdope_t result;

  l4_ipc_send(tid,L4_IPC_SHORT_MSG,1,1,L4_IPC_NEVER,&result);
}

/** receive short message from tid */
void
PREFIX(recv)(l4_threadid_t tid)
{
  l4_msgdope_t result;
  l4_umword_t dummy;

  l4_ipc_receive(tid,L4_IPC_SHORT_MSG,&dummy,&dummy,L4_IPC_NEVER,&result);
}

/** receive short message from ping thread */
void
PREFIX(recv_ping_timeout)(l4_timeout_t timeout)
{
  l4_umword_t dummy;
  l4_msgdope_t result;

  l4_ipc_receive(ping_id,L4_IPC_SHORT_MSG,&dummy,&dummy,timeout,&result);

  if (L4_IPC_ERROR(result) == L4_IPC_RETIMEOUT)
    puts("  >> TIMEOUT!! <<");
}

