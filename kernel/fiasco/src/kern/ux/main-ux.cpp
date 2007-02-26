/*
 * Fiasco-UX
 * Architecture specific main startup/shutdown code
 */

IMPLEMENTATION[ux]:

#include "kernel_console.h"

void main_arch()
{
  // re-enable the console -- it might have been disabled
  // by "-quiet" command line switch
  Kconsole::console()->change_state(0, 0, ~0U, Console::OUTENABLED);
}
