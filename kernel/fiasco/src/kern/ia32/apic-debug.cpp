IMPLEMENTATION[debug]:

#include <cstdio>
#include "simpleio.h"

static
const char*
Apic::reg_lvt_bit_str(unsigned reg, Unsigned32 val, int bit)
{
  static const char * const delivery_mode[] =
    { "fixed", "???", "SMI", "???", "NMI", "INIT", "???", "ExtINT" };
  unsigned bits = 0;

  switch (reg)
    {
    case APIC_LVTT:    bits = MASK | DELIVERY_STATE;			break;
    case APIC_LVT0:
    case APIC_LVT1:    bits = MASK | TRIGGER_MODE | REMOTE_IRR | PIN_POLARITY
			    | DELIVERY_STATE | DELIVERY_MODE;		break;
    case APIC_LVTERR:  bits = MASK | DELIVERY_STATE;			break;
    case APIC_LVTPC:   bits = MASK | DELIVERY_STATE | TRIGGER_MODE;	break;
    case APIC_LVTTHMR: bits = MASK | DELIVERY_STATE | TRIGGER_MODE;	break;
    }

  if (bits & bit == 0)
    return "";

  switch (bit)
    {
    case MASK:
      return val & APIC_LVT_MASKED ? "masked" : "unmasked";
    case TRIGGER_MODE:
      return val & APIC_LVT_LEVEL_TRIGGER ? "level" : "edge";
    case REMOTE_IRR:
      return val & APIC_LVT_REMOTE_IRR ? "IRR" : "";
    case PIN_POLARITY:
      return val & APIC_INPUT_POLARITY ? "active low" : "active high";
    case DELIVERY_STATE:
      return val & APIC_SND_PENDING ? "pending" : "idle";
    case DELIVERY_MODE:
      return delivery_mode[reg_delivery_mode(val)];
    }

  return "";
}

PUBLIC static
void
Apic::reg_show(unsigned reg)
{
  Unsigned32 tmp_val = reg_read(reg);

  printf("%-9s%-6s%-4s%-8s%-7s%02x",
      reg_lvt_bit_str(reg, tmp_val, MASK),
      reg_lvt_bit_str(reg, tmp_val, TRIGGER_MODE),
      reg_lvt_bit_str(reg, tmp_val, REMOTE_IRR),
      reg_lvt_bit_str(reg, tmp_val, DELIVERY_STATE),
      reg_lvt_bit_str(reg, tmp_val, DELIVERY_MODE),
      reg_lvt_vector(tmp_val));
}     

PUBLIC static
void
Apic::regs_show(void)
{
  putstr("\nVectors:   LINT0: "); reg_show(APIC_LVT0);
  putstr("\n           LINT1: "); reg_show(APIC_LVT1);
  putstr("\n           Timer: "); reg_show(APIC_LVTT);
  putstr("\n           Error: "); reg_show(APIC_LVTERR);
  if (have_pcint())
    {
      putstr("\n         PerfCnt: "); 
      reg_show(APIC_LVTPC);
    }
  if (have_tsint())
    {
      putstr("\n         Thermal: ");
      reg_show(APIC_LVTTHMR);
    }
  putchar('\n');
}

PUBLIC static
void
Apic::timer_show(void)
{
  printf("Timer mode: %s  counter: %08x/%08x\n",
	 reg_read(APIC_LVTT) & APIC_LVT_TIMER_PERIODIC
	   ? "periodic" : "one-shot",
	 timer_reg_read_initial(), timer_reg_read());
}

PUBLIC static
void
Apic::id_show(void)
{
  printf("APIC id: %02x version: %02x\n",
	 get_id(), get_version());
}

static
void
Apic::bitfield_show(unsigned reg, const char *name, char flag)
{
  int i, j;
  Unsigned32 tmp_val;

  printf("%-11s    0123456789abcdef0123456789abcdef"
                  "0123456789abcdef0123456789abcdef\n", name);
  for (i=0; i<8; i++)
    {
      if (!(i & 1))
	printf("            %02x ", i*0x20);
      tmp_val = reg_read(reg + i*0x10);
      for (j=0; j<32; j++)
	putchar(tmp_val & (1<<j) ? flag : '.');
      if (i & 1)
	putchar('\n');
    }
}

PUBLIC static
void
Apic::irr_show()
{
  Apic::bitfield_show(APIC_IRR, "Ints Reqst:", 'R');
}

PUBLIC static
void
Apic::isr_show()
{
  Apic::bitfield_show(APIC_ISR, "Ints InSrv:", 'S');
}

