#ifndef __INFTPM_TL_H
#define __INFTPM_TL_H

#include <linux/types.h>

int
tpm_tl_transmit(u16 base_addr, u8 *buf, size_t send_size, size_t recv_size);

#endif
