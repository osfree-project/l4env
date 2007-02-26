INTERFACE:

#include "console.h"
#include "l4_types.h"

class Mem_space;

class Push_console : public Console
{
private:
  static const Unsigned8 *sequence_str;
  static Mword            sequence_len;
  static Mem_space       *sequence_space;
};


IMPLEMENTATION:

#include "keycodes.h"
#include "mem_layout.h"
#include "space.h"

Unsigned8 const *Push_console::sequence_str;
Mword            Push_console::sequence_len;
Mem_space       *Push_console::sequence_space;

static
int
Push_console::get_sequence_byte(Unsigned8 const *s)
{
  return (sequence_space == 0) ? *s : sequence_space->peek_user(s);
}

static
int
Push_console::get_sequence_len()
{
  Unsigned8 const *s = sequence_str;
  int len;

  for (len=0; ; len++)
    if (!get_sequence_byte(s++))
      break;

  return len;
}

PUBLIC
int
Push_console::getchar(bool /*blocking*/)
{
  if (sequence_len)
    {
      sequence_len--;
      int c = get_sequence_byte(sequence_str++);
      // string must not be auto-terminated with KEY_RETURN
      if (c == 0xff)
	sequence_str = 0;
      return c == '_' ? KEY_RETURN : c;
    }

  if (sequence_str)
    {
      // auto-terminate sequence with KEY_RETURN
      return KEY_RETURN;
    }

  return -1; // no keystroke available
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
Push_console::push(Unsigned8 const *str, size_t len, Mem_space *space = 0)
{
  sequence_str    = str;
  sequence_space  = space;
  sequence_len    = len ? len : get_sequence_len();
}

PUBLIC static
void
Push_console::flush(void)
{
  sequence_len = 0;
  sequence_str = 0;
}

PUBLIC
Mword
Push_console::get_attributes() const
{
  return PUSH | IN;
}

