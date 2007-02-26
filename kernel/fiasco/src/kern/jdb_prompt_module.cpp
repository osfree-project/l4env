IMPLEMENTATION:

#include <cstdio>

#include "jdb.h"
#include "jdb_module.h"
#include "static_init.h"


//===================
// Std JDB modules
//===================

/**
 * @brief Jdb-prompt module.
 * 
 * This module handles the 'C' command that
 * changes Jdb prompt settings.
 */
class Jdb_pcm
  : public Jdb_module
{
public:
private:
  char subcmd;
};

static Jdb_pcm jdb_pcm INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

PUBLIC
Jdb_module::Action_code Jdb_pcm::action( int cmd, void *&args, char const *& )
{
  if(cmd!=0)
    return NOTHING;
    
  if(! Jdb::set_prompt_color(*(char*)(args)) )
    {
      putchar(*(char*)(args));
      puts(" - color expected (lLrRgGbByYmMcCwW)!");
    }
  return NOTHING;
}

PUBLIC
int const Jdb_pcm::num_cmds() const
{ 
  return 1;
}

PUBLIC
Jdb_module::Cmd const *const Jdb_pcm::cmds() const
{
  static Cmd cs[] =
    { Cmd( 0, "C", "color", " %c\n", 
	   "C<color>\tset the Jdb prompt color, <color> must be:\n"
	   "\tnN: noir(black), rR: red, gG: green, bB: blue,\n"
	   "\tyY: yellow, mM: magenta, cC: cyan, wW: white;\n"
	   "\tthe capital letters are for bold text.", (void*)&subcmd )
    };

  return cs;
}

PUBLIC
Jdb_pcm::Jdb_pcm()
	: Jdb_module("GENERAL")
{}
