IMPLEMENTATION[ex_regs-arm]:



//Sys_ex_regs_frame::----------------------------------------------------

IMPLEMENT inline 
Mword Sys_ex_regs_frame::lthread() const 
{ 
  return r[0]; 
}

IMPLEMENT inline 
Mword Sys_ex_regs_frame::sp() const 
{ 
  return r[1]; 
}

IMPLEMENT inline 
Mword Sys_ex_regs_frame::ip() const 
{ 
  return r[2]; 
}

IMPLEMENT inline 
L4_uid Sys_ex_regs_frame::preempter() const
{ 
  return L4_uid( r[3] );
}

IMPLEMENT inline 
L4_uid Sys_ex_regs_frame::pager() const
{ 
  return L4_uid( r[12] );
}

IMPLEMENT inline 
void Sys_ex_regs_frame::old_eflags( Mword oefl ) 
{ 
  r[0] = oefl; 
}

IMPLEMENT inline 
void Sys_ex_regs_frame::old_sp( Mword osp ) 
{ 
  r[1] = osp; 
}

IMPLEMENT inline 
void Sys_ex_regs_frame::old_ip( Mword oip ) 
{ 
  r[2] = oip; 
}

IMPLEMENT inline 
void Sys_ex_regs_frame::old_preempter( L4_uid id ) 
{
  r[3] = id.raw();
}

IMPLEMENT inline 
void Sys_ex_regs_frame::old_pager( L4_uid id )
{
  r[12] = id.raw();
}
