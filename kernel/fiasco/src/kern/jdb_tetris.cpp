IMPLEMENTATION:

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "simpleio.h"

#include "jdb.h"
#include "jdb_module.h"
#include "static_init.h"
#include "types.h"
#include "cpu.h"
#include "globals.h"
#include "kernel_console.h"

static int lines, score, current_pos, mode, slice,
           *current_tile, *next_tile, grid[264], next[48];

static int tiles[] = {	9,      9,      -13,    -12,    1,	4,
	                10,     10,     -11,    -12,    -1,	4,
	                11,     13,     -1,     1,      12,	6,
	                3,      3,      -13,    -12,    -1,	2,
	                14,     16,     -1,     11,     1,	5,
	                17,     19,     -1,     13,     1,	5,
	                20,     20,     -1,     1,      2,	1,
	                23,     21,     -12,    11,     13,	3,
	                8,      8,      11,     13,     24,	1,
	                0,      0,      -12,    -1,     11,	4,
	                1,      1,      -12,    1,      13,	4,
	                12,     2,      -12,    1,      12,	6,
	                13,     11,     -12,    -1,     1,	6,
	                2,      12,     -12,    -1,     12,	6,
	                15,     4,      -12,    12,     13,	5,
	                16,     14,     -11,    -1,     1,	5,
	                4,      15,     -13,    -12,    12,	5,
	                18,     5,      -11,    -12,    12,	5,
	                19,     17,     -13,    1,      -1,	5,
	                5,      18,     -12,    12,     11,	5,
	                6,      6,      -12,    12,     24,	1,
	                7,      22,     -13,    1,      11,	3,
	                21,     23,     -13,    -11,    12,	3,
	                22,     7,      -11,    -1,     13,	3 };

static const char *modes[] = { "", "Fiasco Mode", "Lars Mode" };

static int getchar_timeout()
{
  int c;
  static unsigned to = slice;

  while (--to)
    {
      if ((c = Kconsole::console()->getchar (false)) != -1)
        return c;

      Proc::halt();
    }

  to = slice;

  return -1;
}

static void add_score(int value)
{
  if ((score + value) / 1000 != score / 1000)
    slice--;

  if (slice < 0)
      slice = 0;

  score += value;
}

static long unsigned int rand() 
{
  Unsigned64 tsc = Cpu::rdtsc();
  return (tsc >> 2);
}

static int *new_tile()
{
  return tiles + rand() % (7 + mode) * 6;
}

static void show_grid()
{
  int i, j;

  printf ("\033[H");

  for (i = j = 0; i < 264; i++)
    {
      if (grid[i])
        printf ("\033[1;3%dm%c%c", grid[i], 219, 219);
      else
        putstr ("  ");

      if (i % 12 == 11)
        {
          for (; j <= i && j < 48; j++)
            if (next[j])
              printf ("\033[1;3%dm%c%c", next[j], 219, 219);
            else
              putstr ("  ");

          putchar ('\n');
        }
    }  

  printf ("Lines: %d   Score: %d   %s\033[K", lines, score, modes[mode]);
}

static int try_move (int pos, int *tile)
{
  if (grid[pos] + grid[pos+tile[2]] + grid[pos+tile[3]] + grid[pos+tile[4]])
    return 0;

  current_tile = tile;
  current_pos  = pos;

  return 1;
}
 
static void set_grid (int color)
{
  grid[current_pos] =
  grid[current_pos+current_tile[2]] =
  grid[current_pos+current_tile[3]] =
  grid[current_pos+current_tile[4]] = color;
}

static void set_next (int color)
{
  next[17] =
  next[17+next_tile[2]] =
  next[17+next_tile[3]] =
  next[17+next_tile[4]] = color;
}

void show_tile (void)
{
  set_grid(current_tile[5]);
  set_next(next_tile[5]);   

  show_grid();

  set_grid(0);
  set_next(0);
}

/**
 * @brief Jdb-tetris module
 *
 * This module makes fun.
 */
class Jdb_tetris_m 
  : public Jdb_module
{};

static Jdb_tetris_m jdb_tetris_m INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

PUBLIC
Jdb_tetris_m::Jdb_tetris_m()
  : Jdb_module("MISC")
{}

PUBLIC
Jdb_module::Action_code
Jdb_tetris_m::action( int, void *&, char const *&, int & )
{
  int i, j, k, c;

  lines = score = c = 0;
  slice = 300;

  printf ("\033[H\033[J");

  // Setup grid borders
  for (i = 0; i < 264; i++)
    grid[i] = (i % 12 == 0 || i % 12 == 11 || i > 251) ? 7 : 0;

  try_move (17, new_tile());
  next_tile = new_tile();

  while (1)
    {      
      if (c < 0)
        {
          if (!try_move (current_pos + 12, current_tile))
            {
              set_grid(current_tile[5]);

              for (i = k = 0; i < 252; i += 12)
                for (j = i; grid[++j];)
                  if (j - i == 10) {   
                    while (j > i) grid[j--] = 0;            show_grid();
                    while (--j) grid[j + 12] = grid[j];     show_grid();
                    k++;
                  }         

              lines += k;
              add_score (k ? (1 << k) * 10 : 1);

              if (!try_move (17, next_tile))
                break;

              next_tile = new_tile();
            }
        }  

      // Move left
      else if (c == 0x34)
        try_move (current_pos - 1, current_tile);

      // Move right
      else if (c == 0x36)
        try_move (current_pos + 1, current_tile);

      // Drop tile
      else if (c == ' ')
        while (try_move (current_pos + 12, current_tile))
          {
            show_tile();
            add_score(3);
          }
        
      // Left-turn tile
      else if (c == 0x32)
        try_move (current_pos, tiles + 6 ** current_tile);

      // Right-turn tile
      else if (c == 0x38)
        try_move (current_pos, tiles + 6 ** (current_tile+1));

      // Quit
      else if (c == 'q')
        break;

      else if (c == 'm')
        if (++mode == 3)
          mode = 0;

      show_tile();

      c = getchar_timeout();
    }

  printf ("\033[0m");

  return NOTHING;
}

PUBLIC
int const
Jdb_tetris_m::num_cmds() const
{ 
  return 1;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_tetris_m::cmds() const
{
  static Cmd cs[] =
    { Cmd( 0, "X", "X", "",
	   "X\tPlay Tetris (cursor keys = left/right/rotate;\n"
	   "\t[space] = drop; q = quit)", 0 ),
    };

  return cs;
}
