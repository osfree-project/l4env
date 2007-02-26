
INTERFACE:

class Cmdline
{
public:
  static void		init (const char *line);
  static char * const	cmdline();

private:
  static char		_cmdline[256];
};

IMPLEMENTATION:

#include <cstdio>

char Cmdline::_cmdline[256];

IMPLEMENT
void
Cmdline::init (const char *line)
{
  snprintf (_cmdline, sizeof (_cmdline), "%s", line);
}

IMPLEMENT inline
char * const
Cmdline::cmdline()
{
  return _cmdline;
}
