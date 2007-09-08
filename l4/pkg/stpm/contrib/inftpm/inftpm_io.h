#ifndef __INFTPM_IO_H
#define __INFTPM_IO_H

#include <linux/types.h>

int __init
tpm_io_init(u16 base_addr);

int
tpm_io_write(u16 base_addr, u8 value);

int
tpm_io_read(u16 base_addr, u8 *value);

int
tpm_io_clear_fifo(u16 base_addr);

u8
tpm_io_get_status(short base_addr);

int
tpm_io_reset(u16 base_addr);
#endif
