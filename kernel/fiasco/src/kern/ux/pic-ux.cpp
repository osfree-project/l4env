/*
 * Fiasco-UX
 * Architecture specific interrupt code
 */

INTERFACE:

extern int sockets[2];

IMPLEMENTATION[ux]:

#include <fcntl.h>			// for fcntl
#include <pty.h>			// for termios
#include <csignal>			// for SIGIO
#include <cstdio>			// for stdin
#include <cstdlib>			// for EXIT_FAILURE
#include <unistd.h>			// for fork
#include <sys/types.h>			// for fork

#include "boot_info.h"			// for boot_info::fd()
#include "initcalls.h"

int sockets[2];

IMPLEMENT FIASCO_INIT
void 
Pic::init(void)
{
  struct termios tt;

  if (openpty (&sockets[0], &sockets[1], NULL, NULL, NULL)) {
    perror ("openpty");
    return;
  }

  if (tcgetattr (sockets[0], &tt) < 0) {
    perror ("tcgetattr");
    return;
  }
   
  cfmakeraw (&tt);

  if (tcsetattr (sockets[0], TCSADRAIN, &tt) < 0) {
    perror ("tcsetattr");
    return;
  }

  fflush (NULL);

  switch (fork ()) {

    case -1:        
      return;

    case 0:
      break;

    default:
      close (sockets[1]);
      return;
  }

  fclose (stdin);
  fclose (stdout);
  fclose (stderr);

  dup2  (sockets[1], 0);
  close (sockets[0]);
  close (sockets[1]);
  close (Boot_info::fd());

  execl ("irq0", "[I](irq0)", NULL);

  exit (EXIT_FAILURE);
}

IMPLEMENT inline NEEDS [<fcntl.h>, <csignal>, "boot_info.h"]
void Pic::enable_locked (unsigned)
{
  int flags;

  if ((flags = fcntl (sockets[0], F_GETFL)) < 0)
    return;
      
  if (fcntl (sockets[0], F_SETFL, flags | O_NONBLOCK | O_ASYNC) < 0)
    return;

  if (fcntl (sockets[0], F_SETSIG, SIGIO) < 0)
    return;
 
  if (fcntl (sockets[0], F_SETOWN, Boot_info::pid()) < 0)
    return;
}

IMPLEMENT inline
void Pic::disable_locked (unsigned irq)
{}

IMPLEMENT inline
void Pic::acknowledge_locked (unsigned irq)
{}
