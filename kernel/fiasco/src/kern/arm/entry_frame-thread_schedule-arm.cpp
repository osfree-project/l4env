IMPLEMENTATION[thread_schedule-arm]:

//Sys_thread_schedule_frame::----------------------------------------------

IMPLEMENT inline 
L4_sched_param Sys_thread_schedule_frame::param() const
{ 
  return L4_sched_param(r[0]); 
}

IMPLEMENT inline 
L4_uid Sys_thread_schedule_frame::preempter() const 
{
  return L4_uid( r[3] );
}

IMPLEMENT inline 
L4_uid Sys_thread_schedule_frame::dest() const
{
  return L4_uid( r[12] );
}

IMPLEMENT inline 
void Sys_thread_schedule_frame::old_param( L4_sched_param op ) 
{ 
  r[0] = op.raw(); 
}

IMPLEMENT inline 
void Sys_thread_schedule_frame::time( Unsigned64 t ) 
{ 
  r[1] = t; r[2] = t >> 32; 
}

IMPLEMENT inline 
void Sys_thread_schedule_frame::old_preempter( L4_uid id )
{
  r[3] = id.raw();
}

IMPLEMENT inline 
void Sys_thread_schedule_frame::partner( L4_uid id )
{
  r[12] = id.raw();
}
