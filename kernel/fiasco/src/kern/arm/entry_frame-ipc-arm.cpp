IMPLEMENTATION[ipc-arm]:

IMPLEMENT inline 
void Sys_ipc_frame::rcv_source( L4_uid id ) 
{ 
  r[12] = id.raw(); 
}

IMPLEMENT inline 
L4_uid Sys_ipc_frame::rcv_source() 
{ 
  return L4_uid(r[12]);
}

IMPLEMENT inline 
L4_uid Sys_ipc_frame::snd_dest() const
{ 
  return L4_uid(r[12]); 
}

IMPLEMENT inline 
Mword Sys_ipc_frame::has_snd_dest() const
{ 
  return r[12]; 
}

IMPLEMENT inline 
Mword Sys_ipc_frame::irq() const
{ 
  return r[12] -1; 
}

IMPLEMENT inline 
void Sys_ipc_frame::snd_desc( Mword w ) 
{ 
  r[0] =w; 
}

IMPLEMENT inline 
L4_snd_desc Sys_ipc_frame::snd_desc() const 
{ 
  return r[0]; 
}

IMPLEMENT inline 
L4_timeout Sys_ipc_frame::timeout() const 
{ 
  return L4_timeout( r[1] ); 
}

IMPLEMENT inline 
Mword Sys_ipc_frame::msg_word( unsigned index ) const
{
  if(index < 6)
    return r[index+2];
  else
    return 0;
}

IMPLEMENT inline 
void Sys_ipc_frame::set_msg_word( unsigned index, Mword value )
{
  if(index < 6)
    r[index+2] = value;
}

IMPLEMENT inline 
L4_rcv_desc Sys_ipc_frame::rcv_desc() const 
{ 
  return r[11]; 
}

IMPLEMENT inline 
void Sys_ipc_frame::rcv_desc( L4_rcv_desc d ) 
{ 
  r[11] = d.raw(); 
}

IMPLEMENT inline 
L4_msgdope Sys_ipc_frame::msg_dope() const 
{ 
  return L4_msgdope(r[0]); 
}

IMPLEMENT inline
void Sys_ipc_frame::msg_dope_set_error( Mword e )
{
  reinterpret_cast<L4_msgdope*>(&r[0])->error(e);
}

IMPLEMENT inline
unsigned const Sys_ipc_frame::num_reg_words()
{
  return 6;
}

IMPLEMENT inline 
void Sys_ipc_frame::msg_dope( L4_msgdope d ) 
{ 
  r[0] = d.raw(); 
}

IMPLEMENT inline 
void Sys_ipc_frame::msg_dope_combine( L4_msgdope d ) 
{ 
  L4_msgdope m(r[0]);
  m.combine(d);
  r[0] = m.raw(); 
}

IMPLEMENT inline 
void Sys_ipc_frame::copy_msg( Sys_ipc_frame *to ) const 
{
  for(unsigned x = 2; x<2+6; ++x )
    to->r[x] = r[x];
}


