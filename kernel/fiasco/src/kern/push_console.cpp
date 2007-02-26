INTERFACE:

#include "console.h"
#include "l4_types.h"

class Thread;

class Push_console : public Console
{
private:
  static const Unsigned8 *sequence_str;
  static Mword            sequence_len;
  static Thread          *sequence_thread;
};


IMPLEMENTATION:

#include "keycodes.h"
#include "thread.h"

const Unsigned8 *Push_console::sequence_str;
Mword            Push_console::sequence_len;
Thread          *Push_console::sequence_thread;

PUBLIC
int
Push_console::getchar(bool /*blocking*/)
{
  if (sequence_len)
    {
      sequence_len--;
      return sequence_thread 
		? sequence_thread->peek_user((Unsigned8*)sequence_str++)
		: *sequence_str++;
    }

  if (sequence_str)
    {
      // terminate sequence
      sequence_str = 0;
      return KEY_ESC;
    }

  return -1; // unknown
}

PUBLIC
int
Push_console::char_avail() const
{
  return -1; // unknown
}

PUBLIC
int
Push_console::write(char const * /*str*/, size_t len)
{
  return len;
}

PUBLIC static
void
Push_console::push(Unsigned8 const *str, size_t len, Thread *thread)
{
  sequence_str    = str;
  sequence_len    = len;
  sequence_thread = thread;
}

PUBLIC static
void
Push_console::flush(void)
{
  sequence_len = 0;
  sequence_str = 0;
}

PUBLIC
char const *
Push_console::next_attribute (bool restart = false) const
{
  static char const *attribs[] = { "push", "in" };
  static unsigned int pos = 0;

  if (restart)
    pos = 0;

  if (pos < sizeof (attribs) / sizeof (*attribs))
    return attribs[pos++];

  return 0;
}

