IMPLEMENTATION[unmap-arm]:

//Sys_unmap_frame::---------------------------------------------------

IMPLEMENT inline 
L4_fpage Sys_unmap_frame::fpage() const
{ 
  return L4_fpage(r[0]); 
}

IMPLEMENT inline 
Mword Sys_unmap_frame::map_mask() const
{ 
  return r[1]; 
}

IMPLEMENT inline 
bool Sys_unmap_frame::downgrade() const
{ 
  return !(r[1] & 2); 
}

IMPLEMENT inline 
bool Sys_unmap_frame::self_unmap() const
{ 
  return r[1] & 0x80000000; 
}
