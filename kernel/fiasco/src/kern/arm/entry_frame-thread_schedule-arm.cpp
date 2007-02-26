IMPLEMENTATION[thread_schedule-arm]:

//Sys_thread_schedule_frame::----------------------------------------------

IMPLEMENT inline
Unsigned64 Sys_thread_schedule_frame::time() const
{
  return (Unsigned64) r[4] << 32 | (Unsigned64) r[3];
}

IMPLEMENT inline 
L4_sched_param Sys_thread_schedule_frame::param() const
{ 
  return L4_sched_param(r[0]); 
}

IMPLEMENT inline 
L4_uid Sys_thread_schedule_frame::preempter() const 
{
  return L4_uid( r[2] );
}

IMPLEMENT inline 
L4_uid Sys_thread_schedule_frame::dest() const
{
  return L4_uid( r[1] );
}

IMPLEMENT inline 
void Sys_thread_schedule_frame::old_param( L4_sched_param op ) 
{ 
  r[0] = op.raw(); 
}

IMPLEMENT inline 
void Sys_thread_schedule_frame::time( Unsigned64 t ) 
{ 
  r[3] = t; r[4] = t >> 32; 
}

IMPLEMENT inline 
void Sys_thread_schedule_frame::old_preempter( L4_uid id )
{
  r[2] = id.raw();
}

IMPLEMENT inline 
void Sys_thread_schedule_frame::partner( L4_uid id )
{
  r[1] = id.raw();
}
