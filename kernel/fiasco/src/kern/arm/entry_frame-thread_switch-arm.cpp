IMPLEMENTATION[thread_switch-arm]:

//Sys_thread_switch_frame::-------------------------------------------------

IMPLEMENT inline 
L4_uid Sys_thread_switch_frame::dest() const
{ 
  //warning this is against the spec, where only id.low in esi is significant
  return L4_uid( r[12] );
}

IMPLEMENT inline 
Mword Sys_thread_switch_frame::has_dest() const
{ 
  return r[12]; 
}
