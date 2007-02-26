IMPLEMENTATION[sa1100]:

#include <cstring>
#include <cstdio>

#include "boot_info.h"
#include "config.h"
#include "initcalls.h"
#include "sa_1100.h"

IMPLEMENT FIASCO_INIT
void Pic::init()
{
  // only unmasked interrupts wakeup from idle
  Sa1100::hw_reg(0x01, Sa1100::ICCR);
  // mask all interrupts
  Sa1100::hw_reg(0x00, Sa1100::ICMR);
  // all interrupts are IRQ's (no FIQ)
  Sa1100::hw_reg(0x00, Sa1100::ICLR);
}

IMPLEMENT inline NEEDS["sa_1100.h"]
void Pic::disable_locked( unsigned irq )
{
  Sa1100::hw_reg(Sa1100::hw_reg(Sa1100::ICMR) & ~(1<<irq), Sa1100::ICMR);
}

IMPLEMENT inline NEEDS["sa_1100.h"]
void Pic::enable_locked( unsigned irq )
{
  Sa1100::hw_reg(Sa1100::hw_reg(Sa1100::ICMR) | (1<<irq), Sa1100::ICMR);
}

IMPLEMENT inline 
void Pic::acknowledge_locked( unsigned /*irq*/ )
{}


IMPLEMENT inline NEEDS["sa_1100.h"]
Pic::Status Pic::disable_all_save()
{
  Status s;
  s  = Sa1100::hw_reg(Sa1100::ICMR);
  Sa1100::hw_reg(0, Sa1100::ICMR);
  return s;
}

IMPLEMENT inline NEEDS["sa_1100.h"]
void Pic::restore_all( Status s )
{
  Sa1100::hw_reg(s, Sa1100::ICMR);
}
