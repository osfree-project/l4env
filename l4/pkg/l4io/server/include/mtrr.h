#ifndef __L4IO_SERVER_INCLUDE_MTRR_H_
#define __L4IO_SERVER_INCLUDE_MTRR_H_

enum
{
  MTRR_UC    = 0x00, /* Uncacheable */
  MTRR_WC    = 0x01, /* Write Combining */
  MTRR_WT    = 0x04, /* Write-through */
  MTRR_WP    = 0x05, /* Write-protected */
  MTRR_WB    = 0x06, /* Writeback */
};

int mtrr_init(void);
int mtrr_set(l4_addr_t addr, l4_size_t size, int type);

#endif
