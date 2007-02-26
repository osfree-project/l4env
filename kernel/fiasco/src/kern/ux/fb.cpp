//
// Framebuffer handling for Fiasco/UX
//

INTERFACE:

class Fb
{
public:
  static void init();
  static void enable();

private:
  static void bootstrap();
};

IMPLEMENTATION:

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include "boot_info.h"
#include "initcalls.h"
#include "pic.h"

IMPLEMENT FIASCO_INIT
void
Fb::bootstrap()
{
  char s_fd[3];
  char s_width[5];
  char s_height[5];
  char s_depth[3];
  char s_fbaddr[15];

  snprintf (s_fd,     sizeof (s_fd),     "%u", Boot_info::fd());
  snprintf (s_fbaddr, sizeof (s_fbaddr), "%u", Boot_info::fb_phys());
  snprintf (s_width,  sizeof (s_width),  "%u", Boot_info::fb_width());
  snprintf (s_height, sizeof (s_height), "%u", Boot_info::fb_height());
  snprintf (s_depth,  sizeof (s_depth),  "%u", Boot_info::fb_depth());

  execl (Boot_info::fb_program(), Boot_info::fb_program(),
         "-f", s_fd, "-s", s_fbaddr,
         "-x", s_width, "-y", s_height,
         "-d", s_depth, NULL);
}

IMPLEMENT FIASCO_INIT
void
Fb::init()
{
  // Check if frame buffer is available
  if (!Boot_info::fb_depth())
    return;

  // Setup virtual interrupt
  if (!Pic::setup_irq_prov (Pic::IRQ_CON,
                            Boot_info::fb_program(), bootstrap))
    {
      puts ("Problems setting up console interrupt!");
      exit (1);
    }

  printf ("Starting Framebuffer: %ux%u@%u\n\n",
          Boot_info::fb_width(), Boot_info::fb_height(),
          Boot_info::fb_depth());
}

IMPLEMENT inline NEEDS ["pic.h"]
void
Fb::enable()
{
  // Check if frame buffer is available
  if (!Boot_info::fb_depth())
    return;

  Pic::enable (Pic::IRQ_CON);
}
