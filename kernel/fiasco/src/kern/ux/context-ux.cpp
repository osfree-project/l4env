INTERFACE[ux-segments]:

EXTENSION class Context
{
protected:
  Unsigned32	_es, _fs, _gs;
};


IMPLEMENTATION[ux]:

IMPLEMENT inline
void
Context::switch_fpu (Context *)
{}
