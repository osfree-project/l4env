IMPLEMENTATION[id_nearest-arm]:

//Entry_id_nearest_data::-------------------------------------------------

IMPLEMENT inline 
L4_uid Sys_id_nearest_frame::dest() const
{ 
  return L4_uid( r[0] );
}

IMPLEMENT inline
void Sys_id_nearest_frame::type( Mword type )
{
  r[0] = type;
}

IMPLEMENT inline 
void Sys_id_nearest_frame::nearest( L4_uid id ) 
{ 
  r[1] = id.raw(); 
}

