IMPLEMENTATION[arm]:

IMPLEMENT inline
void Fpu::init_state( Fpu_state *s ) 
{}

IMPLEMENT inline
unsigned const Fpu::state_size()
{
  return 0;
}

IMPLEMENT inline
unsigned const Fpu::state_align()
{
  return 0;
}

IMPLEMENT
void Fpu::init() 
{}
   
IMPLEMENT inline
void Fpu::save_state( Fpu_state * ) 
{}  


IMPLEMENT inline
void Fpu::restore_state( Fpu_state * ) 
{}

IMPLEMENT inline
void Fpu::disable()
{}

IMPLEMENT inline
void Fpu::enable()
{}

