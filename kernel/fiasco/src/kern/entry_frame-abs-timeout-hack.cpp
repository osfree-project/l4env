IMPLEMENTATION:

IMPLEMENT inline
Mword Sys_ipc_frame::has_abs_snd_timeout() const
{
  return snd_dst().abs_snd_timeout();
}
IMPLEMENT inline
Mword Sys_ipc_frame::abs_snd_clock() const
{
  return snd_dst().abs_snd_clock();
}

IMPLEMENT inline
Mword Sys_ipc_frame::has_abs_rcv_timeout() const
{
  return snd_dst().abs_rcv_timeout();
}

IMPLEMENT inline
Mword Sys_ipc_frame::abs_rcv_clock() const
{
  return snd_dst().abs_rcv_clock();
}

