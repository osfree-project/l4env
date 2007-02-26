
/*
 * JDB Module implementing ANSI/vt100 functions
 */

INTERFACE:

EXTENSION class Jdb
{
public:
  static void cursor (unsigned int row=0, unsigned int col=0);
  static void cursor_save();
  static void cursor_restore();
  static void screen_erase();
  static void screen_scroll (unsigned int start, unsigned int end);
};

IMPLEMENTATION[ansi]:

#include <cstdio>

IMPLEMENT
void
Jdb::cursor (unsigned int row=0, unsigned int col=0)
{
  if (row || col)
    printf ("\033[%u;%uH", row, col);
  else
    printf ("\033[%u;%uH", 1, 1);
}

IMPLEMENT
void
Jdb::cursor_save()
{
  printf ("\0337");
}

IMPLEMENT
void
Jdb::cursor_restore()
{
  printf ("\0338");
}

IMPLEMENT
void
Jdb::screen_erase()
{
  printf ("\033[2J");
}   
   
IMPLEMENT
void
Jdb::screen_scroll (unsigned int start, unsigned int end)
{
  if (start || end)
    printf ("\033[%u;%ur", start, end);
  else
    printf ("\033[r");
} 
