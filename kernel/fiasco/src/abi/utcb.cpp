INTERFACE:

class Utcb
{
};

IMPLEMENTATION:

PUBLIC
bool Utcb::inherit_fpu() const
{ return buffers[0] & 2; }

