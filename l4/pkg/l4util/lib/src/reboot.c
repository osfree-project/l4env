#include <l4/sys/kdebug.h>
#include <l4/sigma0/kip.h>
#include <l4/util/reboot.h>

#include <string.h>

#include "reboot_arch.h"

void
l4util_reboot(void)
{
  l4_kernel_info_t *kip;

  /* First try UX which always is "rebooted" via JDB */
  if ((kip = l4sigma0_kip_map(L4_INVALID_ID)))
    {
      if (l4sigma0_kip_kernel_is_ux())
	{
	  enter_kdebug("*#^");          /* Always available */

	  enter_kdebug("Exit failed!"); /* Should we loop here? */
	}
    }

  /* Machine dependent reboot */
  l4util_reboot_arch();
}
