IMPLEMENTATION [arm]:

/* This is a more reliable delay than a few short jmps. */
IMPLEMENT inline
void Io::iodelay() 
{}

IMPLEMENT inline
Unsigned8  Io::in8 ( unsigned long port )
{
  return *(volatile Unsigned8 *)(port*4);
}

IMPLEMENT inline
Unsigned16 Io::in16( unsigned long port )
{
  return *(volatile Unsigned16 *)(port*4);
}

IMPLEMENT inline
Unsigned32 Io::in32( unsigned long port )
{
  return *(volatile Unsigned32 *)(port*4);
}

IMPLEMENT inline
void Io::out8 ( Unsigned8  val, unsigned long port )
{
  *(volatile Unsigned8 *)(port*4) = val;
}

IMPLEMENT inline
void Io::out16( Unsigned16 val, unsigned long port )
{
  *(volatile Unsigned16 *)(port*4) = val;
}

IMPLEMENT inline
void Io::out32( Unsigned32 val, unsigned long port )
{
  *(volatile Unsigned32 *)(port*4) = val;
}


