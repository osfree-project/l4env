IMPLEMENTATION:

#include <cstdio>
#include "simpleio.h"

#include "jdb_module.h"
#include "static_init.h"
#include "terminate.h"
#include "types.h"


//===================
// Std JDB modules
//===================

/**
 * @brief Private 'exit' module.
 * 
 * This module handles the 'exit' or '^' command that
 * makes a call to exit() and virtually reboots the system.
 */
class Jdb_exit_module : public Jdb_module
{
public:
};

static Jdb_exit_module jdb_exit_module INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

PUBLIC
Jdb_module::Action_code Jdb_exit_module::action( int cmd, void *&, 
						 char const *&, int & )
{
  if(cmd!=0)
    return NOTHING;

  putstr("\033[1;127r\033[127;1H");

  terminate(1);

}

PUBLIC
int const Jdb_exit_module::num_cmds() const
{ 
  return 1;
}

PUBLIC
Jdb_module::Cmd const *const Jdb_exit_module::cmds() const
{
  static Cmd cs[] =
    { Cmd( 0, "^", "exit", "", 
	   "^\treboot the system", (void*)0 )
    };

  return cs;
}

PUBLIC
Jdb_exit_module::Jdb_exit_module()
  : Jdb_module("GENERAL")
{}
