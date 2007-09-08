/*
 * include/linux/tpm.h
 *
 * Device driver for TCPA TPM (trusted platform module).
 */
#ifndef _LINUX_TPM_H
#define _LINUX_TPM_H

#include <linux/ioctl.h>

/* ioctl commands */
#define	TPMIOC_CANCEL		_IO('T', 0x00)

#endif /* _LINUX_TPM_H */
