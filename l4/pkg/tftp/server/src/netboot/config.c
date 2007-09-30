/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

#include	"grub.h"
#include	"nic.h"

extern int ore_probe(struct dev *dev);

int probe(struct dev *dev)
{
  EnterFunction("probe");

  if (dev->how_probe == PROBE_FIRST) {
    dev->to_probe = PROBE_ORE;
    memset(&dev->state, 0, sizeof(dev->state));
  }
  if (dev->to_probe == PROBE_ORE)
    dev->how_probe = ore_probe(dev);
  else
    dev->how_probe = PROBE_FAILED;

  LeaveFunction("probe");
  return dev->how_probe;
}

void disable(struct dev *dev)
{
  if (dev->disable) {
    dev->disable(dev);
    dev->disable = 0;
  }
}
