IMPLEMENTATION[abs-timeout-hack]:

IMPLEMENT inline
Mword Sys_ipc_frame::has_abs_snd_timeout() const
{
  return snd_dest().abs_send_timeout();
}
IMPLEMENT inline
Mword Sys_ipc_frame::abs_snd_clock() const
{
  return snd_dest().abs_send_clock();
}

IMPLEMENT inline
Mword Sys_ipc_frame::has_abs_rcv_timeout() const
{
  return snd_dest().abs_recv_timeout();
}

IMPLEMENT inline
Mword Sys_ipc_frame::abs_rcv_clock() const
{
  return snd_dest().abs_recv_clock();
}
