#include "l4io_info.offs.h"

	.section .rospace, "a", %nobits
	.p2align L4_LOG2_PAGESIZE
	.size	io_info, L4_PAGESIZE
	.type	io_info, %object
	.global	io_info
io_info:
        .space  L4_PAGESIZE

	.global	jiffies
	.global	ticks
	.global	HZ
	.global	xtime
jiffies  = io_info + INFO_JIFFIES
ticks	 = io_info + INFO_JIFFIES
HZ	 = io_info + INFO_HZ
xtime    = io_info + INFO_XTIME


