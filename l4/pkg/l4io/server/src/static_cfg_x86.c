
#include "static_cfg.h"


static l4io_desc_device_t
	tpm_tis = { "TPM TIS", 1, { { L4IO_RESOURCE_MEM, 0xfed40000, 0xfed44fff } } };

register_device_group("x86", &tpm_tis);
