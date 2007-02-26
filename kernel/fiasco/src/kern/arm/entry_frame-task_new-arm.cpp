IMPLEMENTATION[task_new-arm]:

//Sys_task_new_frame::-------------------------------------------

IMPLEMENT inline 
Mword Sys_task_new_frame::mcp() const 
{ 
  return r[0];
}

IMPLEMENT inline 
L4_uid Sys_task_new_frame::new_chief() const
{ 
  return L4_uid( r[0] ); 
}

IMPLEMENT inline   
Mword Sys_task_new_frame::sp() const
{ 
  return r[1];
}

IMPLEMENT inline 
Mword Sys_task_new_frame::ip() const
{ 
  return r[2];
}
  
IMPLEMENT inline 
Mword Sys_task_new_frame::has_pager() const
{
  return r[3];
}

IMPLEMENT inline 
L4_uid Sys_task_new_frame::pager() const
{
  return L4_uid( r[3] );    
}

IMPLEMENT inline 
L4_uid Sys_task_new_frame::dest() const
{
  return L4_uid( r[12] );    
}

IMPLEMENT inline 
void Sys_task_new_frame::new_taskid( L4_uid id ) 
{
  r[12] = id.raw(); 
}
