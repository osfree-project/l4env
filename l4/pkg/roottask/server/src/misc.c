/**
 * \file	roottask/server/src/misc.c
 * \brief	Miscelaneous (support functions) functions.
 *
 * \date	03/2005
 * \author	Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 **/
#include <ctype.h>
#include <string.h>

#include "misc.h"

/**
 * Check if the string program is used as a binary name in the
 * module_cmdline string.
 *
 * module_cmdline can be something like this:
 *  (nd)/tftpboot/user/binary param1 param2
 *  (nd)/tftpboot/user/binary
 *  /home/user/dev/l4/.../binary
 *  program
 * program is something like:
 *  program
 * And we want to know whether program is used in the path.
 */
int
is_program_in_cmdline(const char *module_cmdline, const char *program)
{
  int end, i, j;

  /* Find first space character or end of line and remember this in "end" */
  for (end = 0; module_cmdline[end] && !isspace(module_cmdline[end]); end++)
    ;

  /* Compare strings backwards */
  i = end;
  j = strlen(program);
  while (i && j)
    {
      i--;
      j--;
      if (module_cmdline[i] != program[j])
	return 0;
    }
  /* If there's something left in program, it didn't match completely */
  if (j)
    return 0;
  /* If there's something left in module_cmdline, then it must be a '/' */
  if (i > 0 && module_cmdline[i - 1] != '/')
    return 0;

  return 1; /* Ok, it matched! */
}
