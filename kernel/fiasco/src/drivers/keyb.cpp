INTERFACE:

#include "console.h"

/**
 * @brief Platform independent keyboard stub.
 *
 * Provides an empty implentation for write(...).
 */
class Keyb : public Console
{
public:
  // must be implemented in platform part.
  int getchar( bool blocking = true );

  // implemented empty
  int write( char const *str, size_t len );


};


IMPLEMENTATION:

IMPLEMENT
int Keyb::write( char const *, size_t len)
{
  return len;
}

PUBLIC
char const *Keyb::next_attribute( bool restart = false ) const
{
  static char const *attribs[] = { "direct", "in", 0 };
  static unsigned pos = 0;
  if(restart)
    pos = 0;
  if(pos<3)
    return attribs[pos++];
  else
    return 0;
}
