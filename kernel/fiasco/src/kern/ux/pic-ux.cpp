/*
 * Fiasco-UX
 * Architecture specific interrupt code
 */

INTERFACE:

#include "config.h"
#include <sys/poll.h>

EXTENSION class Pic
{
public:
  static int		irq_pending();
  static void		eat(unsigned);
  static void		set_owner(int);
  static unsigned int	map_irq_to_gate(unsigned);
  static bool		setup_irq_prov (unsigned irq,
                                        const char * const path,
					void (*bootstrap_func)());
  static void		irq_prov_shutdown();
  static void           snd_to_irq( unsigned irq, Mword w1, Mword w2 );

  enum {
    IRQ_TIMER = 0,
    IRQ_CON   = 1,
  };

private:
  static unsigned int	highest_irq;
  static unsigned int	pids[Config::Max_num_irqs];
  static struct pollfd	pfd[Config::Max_num_irqs];
};

IMPLEMENTATION[ux]:

#include <cassert>			// for assert
#include <csignal>			// for SIGIO
#include <cstdio>			// for stdin
#include <cstdlib>			// for EXIT_FAILURE
#include <fcntl.h>			// for fcntl
#include <pty.h>			// for termios
#include <unistd.h>			// for fork
#include <sys/types.h>			// for fork

#include "boot_info.h"			// for boot_info::fd()
#include "initcalls.h"

unsigned int	Pic::highest_irq;
unsigned int	Pic::pids[Config::Max_num_irqs];
struct pollfd	Pic::pfd[Config::Max_num_irqs];

IMPLEMENT FIASCO_INIT
void 
Pic::init()
{
  atexit (&irq_prov_shutdown);
}

IMPLEMENT
bool
Pic::setup_irq_prov (unsigned irq, const char * const path,
                     void (*bootstrap_func)())
{
  struct termios tt;
  int sockets[2];

  if (access (path, X_OK | F_OK))
    {
      perror (path);
      return false;
    }

  if (openpty (&sockets[0], &sockets[1], NULL, NULL, NULL))
    {
      perror ("openpty");
      return false;
    }

  if (tcgetattr (sockets[0], &tt) < 0)
    {
      perror ("tcgetattr");
      return false;
    }
   
  cfmakeraw (&tt);

  if (tcsetattr (sockets[0], TCSADRAIN, &tt) < 0)
    {
      perror ("tcsetattr");
      return false;
    }

  fflush (NULL);

  switch (pids[irq] = fork())
    {
      case -1:        
        return false;

      case 0:
        break;

      default:
        close (sockets[1]);
        fcntl (sockets[0], F_SETFD, FD_CLOEXEC);
        pfd[irq].fd = sockets[0];
        return true;
    }

  // Unblock all signals except SIGINT, we enter jdb with it and don't want
  // the irq providers to die
  sigset_t mask;
  sigemptyset (&mask);
  sigaddset   (&mask, SIGINT);
  sigprocmask (SIG_SETMASK, &mask, NULL);

  fclose (stdin);
  fclose (stdout);
  fclose (stderr);

  dup2  (sockets[1], 0);
  close (sockets[0]);
  close (sockets[1]);
  bootstrap_func();
    
  _exit (EXIT_FAILURE);
} 

IMPLEMENT
void
Pic::irq_prov_shutdown()
{
  for (unsigned int i = 0; i < highest_irq; i++)
    if (pfd[i].events & POLLIN)
      kill(pids[i], SIGTERM);
}

IMPLEMENT
void Pic::snd_to_irq( unsigned irq, Mword w1, Mword w2 )
{
  Mword buf[2] = {w1,w2};
  if(pids[irq])
    write( pfd[irq].fd, buf, sizeof(buf) );
}

IMPLEMENT inline NEEDS [<cassert>, <csignal>, <fcntl.h>, "boot_info.h"]
void
Pic::enable_locked (unsigned irq, unsigned /*prio*/)
{
  int flags;

  // If fd is zero, someone tried to activate an IRQ without provider
  assert (pfd[irq].fd);

  if ((flags = fcntl (pfd[irq].fd, F_GETFL)) < 0			||
       fcntl (pfd[irq].fd, F_SETFL, flags | O_NONBLOCK | O_ASYNC) < 0	||
       fcntl (pfd[irq].fd, F_SETSIG, SIGIO) < 0				||
       fcntl (pfd[irq].fd, F_SETOWN, Boot_info::pid()) < 0)
    return;

  pfd[irq].events = POLLIN;
  
  if (irq >= highest_irq)
    highest_irq = irq + 1;
}

IMPLEMENT inline
void
Pic::disable_locked (unsigned)
{}

IMPLEMENT inline
void
Pic::acknowledge_locked (unsigned)
{}

IMPLEMENT
int
Pic::irq_pending()
{
  unsigned int i;
  
  for (i = 0; i < highest_irq; i++)
    pfd[i].revents = 0;

  if (poll (pfd, highest_irq, 0) > 0)
    for (i = 0; i < highest_irq; i++)
      if (pfd[i].revents & POLLIN)
        return i;

  return -1;
}

IMPLEMENT inline NEEDS [<cassert>, <unistd.h>]
void
Pic::eat (unsigned irq)
{
  char buffer[8];
  
  assert (pfd[irq].events & POLLIN);
  
  while (read (pfd[irq].fd, buffer, sizeof (buffer)) > 0)
    ;
}

/*
 * Possible problem if an IRQ gets enabled and the system is already
 * long running and the owner is set wrong?
 */
IMPLEMENT inline
void
Pic::set_owner (int pid)
{
  for (unsigned int i = 0; i < highest_irq; i++)
    if (pfd[i].events & POLLIN)
      fcntl (pfd[i].fd, F_SETOWN, pid);
}

IMPLEMENT inline
unsigned int
Pic::map_irq_to_gate (unsigned irq)
{
  return 0x20 + irq;
}
