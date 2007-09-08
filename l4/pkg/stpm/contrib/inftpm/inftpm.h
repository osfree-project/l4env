#ifndef __INFTPM_H
#define __INFTPM_H

// the minor number from the ibm driver
#define TPM_MINOR_NR			224
#define	MODULE_NAME			"inftpm"

#define ICH4_IO_BASE_ADDR            0x1400
#define ICH5_IO_BASE_ADDR	     0x4700
#define ICH4_CONFIG_ADDR             0xE0 
#define ICH5_CONFIG_ADDR	     0x4E


// PCI configuration addresses
#define	PCI_GEN1_DEC			0xE4
#define	PCI_GEN2_DEC			0xEC

// identification info
#define TPM_DEV_ID_VALUE                0x0006
#define TPM_DEV_VEN_VALUE               0x15D1
#define TPM_DEV_PCI_VALUE               0x0006


#define	TPM_IOCTL_RESET		_IO('T', 0x01)
#define	TPM_IOCTL_STATUS	_IO('T', 0x02)
#define	TPM_IOCTL_DEV_INIT	_IO('T', 0x03)


#define error(X,...) printk(KERN_ERR MODULE_NAME " %s() " X "\n", __func__ ,##__VA_ARGS__)

#ifdef DEBUG
  #define debug(X,...) printk(KERN_INFO MODULE_NAME " %s() " X "\n", __func__ ,##__VA_ARGS__)
#else
  #define debug(X,...)
#endif

#endif
