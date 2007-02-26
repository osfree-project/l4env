IMPLEMENTATION:

#include <cstdio>
#include "simpleio.h"

#include "jdb.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "kernel_console.h"
#include "static_init.h"
#include "terminate.h"
#include "types.h"


/**
 * Private 'exit' module.
 * 
 * This module handles the 'exit' or '^' command that
 * makes a call to exit() and virtually reboots the system.
 */
class Jdb_exit_module : public Jdb_module
{
};

static Jdb_exit_module jdb_exit_module INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

PUBLIC
Jdb_module::Action_code
Jdb_exit_module::action (int cmd, void *&, char const *&, int &)
{
  if (cmd!=0)
    return NOTHING;

  // re-enable output of all consoles but GZIP and DEBUG
  Kconsole::console()->change_state(0, Console::GZIP | Console::DEBUG,
				    ~0U, Console::OUTENABLED);
  // re-enable output of all consoles but PUSH and DEBUG
  Kconsole::console()->change_state(0, Console::PUSH | Console::DEBUG,
				    ~0U, Console::INENABLED);

  Jdb::screen_scroll(1, 127);
  Jdb::blink_cursor(Jdb_screen::height(), 1);
  Jdb::cursor(127, 1);
  terminate(1);
  return LEAVE;
}

PUBLIC
int const
Jdb_exit_module::num_cmds() const
{ 
  return 1;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_exit_module::cmds() const
{
  static Cmd cs[] =
    { { 0, "^", "exit", "", "^\treboot the system", (void*)0 } };

  return cs;
}

PUBLIC
Jdb_exit_module::Jdb_exit_module()
  : Jdb_module("GENERAL")
{}
