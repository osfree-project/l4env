IMPLEMENTATION:

#include <cstdio>

#include "io.h"
#include "jdb_module.h"
#include "jdb.h"
#include "pic.h"
#include "static_init.h"

//===================
// Std JDB modules
//===================

/**
 * @brief Private IA32-I/O module.
 */
class Io_m 
  : public Jdb_module
{
public:

private:
  char porttype;

  struct Port_io_buf {
    unsigned adr;
    unsigned value;
  };

  struct Pci_port_buf {
    unsigned bus;
    unsigned dev;
    unsigned reg;
  };
  
  struct Irq_buf {
    unsigned irq;
  };

  union Input_buffer {
    Port_io_buf io;
    Pci_port_buf pci;
    Irq_buf irq;
  };

  Input_buffer buf;
  

};

static Io_m io_m INIT_PRIORITY(JDB_MODULE_INIT_PRIO);


char const *const port_in_fmt = " address: %p";
char const *const port_out_fmt = " address: %p, value: %i\n";
char const *const pci_in_fmt = " pci config dword - bus: %p, dev: %p, reg: %p" ;
char const *const ack_irq_fmt = " ack IRQ: %x\n";
char const *const mask_irq_fmt = " mask IRQ: %x";
char const *const unmask_irq_fmt = " unmask IRQ: %x";


#define PCI_CONFIG_ADDR	0xcf8
#define PCI_CONFIG_DATA	0xcfc

#if 0
static
int
Io_m::get_pci_bus_dev_reg(unsigned *bus, unsigned *dev, unsigned *reg)
{
  printf("pci config dword - bus:");
  bc.x += 23;
  if ((*bus = get_value(8, HEX_VALUE)) == 0xffffffff)
    return false;
  printf(" dev:");
  bc.x += 5;
  if ((*dev = get_value(8, HEX_VALUE)) == 0xffffffff)
    return false;
  printf(" reg:");
  bc.x += 5;
  if ((*reg = get_value(8, HEX_VALUE)) == 0xffffffff)
    return false;

  return true;
}
#endif


PUBLIC
Jdb_module::Action_code Io_m::action( int cmd, void *&args, char const *&fmt )
{
  
  static char const *const port_in_fmt = " address: %p";
  static char const *const port_out_fmt = " address: %p, value: %i\n";
  static char const *const pci_in_fmt = " pci config dword - bus: %p, dev: %p, reg: %p" ;
  static char const *const ack_irq_fmt = " ack IRQ: %x\n";
  static char const *const mask_irq_fmt = " mask IRQ: %x";
  static char const *const unmask_irq_fmt = " unmask IRQ: %x";


  unsigned answer = 0xffffffff;
  
  if(args != (void*)&buf) 
    {
      args = &buf;
      if(cmd==0) // in
				{
					switch(porttype)
						{
						case '1':
						case '2':
						case '4':
							fmt = port_in_fmt;
							return EXTRA_INPUT;
							
						case 'p':
							fmt = pci_in_fmt;
							return EXTRA_INPUT;
							
						default:
							printf(" - unknown port type (must be 1,2,4, or p)\n");
							return NOTHING;
						}
				}
      else // out
				{
					switch(porttype)
						{
						case '1':
						case '2':
						case '4':
							fmt = port_out_fmt;
							return EXTRA_INPUT;
	    
						case 'a':
							fmt = ack_irq_fmt;
							return EXTRA_INPUT;

						case 'm':
							fmt = mask_irq_fmt;
							return EXTRA_INPUT;

						case 'u':
							fmt = unmask_irq_fmt;
							return EXTRA_INPUT;

						default:
							printf(" - unknown port type (must be 1,2,4,a,m, or u)\n");
							return NOTHING;
						}

				}
    }
  else
    {
      switch(porttype)
				{
				case '1':
				case '2':
				case '4':
					{
						if (buf.io.adr == 0xffffffff)
							return NOTHING;
						if(cmd==0) // in
							printf(" => ");

						switch (porttype)
							{
							case '1':
								if(cmd==0) // in
									{
										if (buf.io.adr == Pic::MASTER_OCW)
											printf("%02x - (shadow of master-PIC register)\n", Jdb::pic_status & 0x0ff);
										else if (buf.io.adr == Pic::SLAVES_OCW)
											printf("%02x - (shadow of slave-PIC register)\n", Jdb::pic_status >> 8);
										else
											{
#if 0
												// CRTC index register may be overwritten by blink_cursor()
												if ((vga_crtc_idx != 0) && (buf.io.adr == vga_crtc_idx+1))
													Io::out8( vga_crtc_idx_val, vga_crtc_idx);
												if ((vga_crtc_idx != 0) && (buf.io.adr == vga_crtc_idx))
													answer = vga_crtc_idx_val;
												else
#endif
													answer = Io::in8(buf.io.adr);
												printf("%02x\n", (int)answer);
											}
									}
								else if(cmd == 1) // out
									{
										if (buf.io.adr == Pic::MASTER_OCW)
											{
												Jdb::pic_status = (Jdb::pic_status & 0x0ff00) | buf.io.value;
												printf(" - master-PIC mask register will be set on <g>\n");
											}
										else if (buf.io.adr == Pic::SLAVES_OCW)
											{
												Jdb::pic_status = (Jdb::pic_status & 0x0ff) | (buf.io.value<<8);
												printf(" - slave-PIC mask register will be set on <g>\n");
											}
										else 
											{
#if 0
												if (buf.io.adr == c_port)
													{
														// CRTC register may be overwritten by blink_cursor()
														vga_crtc_idx     = buf.io.adr;
														vga_crtc_idx_val = buf.io.value;
														break;
													}
												else if ((vga_crtc_idx != 0) && (buf.io.adr == vga_crtc_idx+1))
													{
														Io::out8( vga_crtc_idx_val, vga_crtc_idx);
													}
#endif 
												Io::out8( buf.io.value, buf.io.adr );
											}
									}
								break;
							case '2':
								if(cmd==0) // in
									{
										answer = Io::in16(buf.io.adr);
										printf("%04x\n", answer);
									} 
								else
									Io::out16( buf.io.value, buf.io.adr );
								break;
							case '4': 
								if(cmd==0) // in
									{
										answer = Io::in32(buf.io.adr);
										printf("%08x\n", answer);
									}
								else
									Io::out32( buf.io.value, buf.io.adr );
								break;
							}
					}
					break;
				case 'p':
					printf(" => ");
      
					Io::out32( 0x80000000 | (buf.pci.bus<<16) | (buf.pci.dev<<8) 
										 | (buf.pci.reg & ~3),PCI_CONFIG_ADDR);
					answer = Io::in32(PCI_CONFIG_DATA);
					printf("0x%08x\n", answer);
					break;

				case 'a':
	  
					if (buf.irq.irq < 8)
						Io::out8( 0x60 + buf.irq.irq, Pic::MASTER_ICW);
					else 
						{
							Io::out8( 0x60 + (buf.irq.irq & 7), Pic::SLAVES_ICW);
							Io::out8( 0x60 + 2, Pic::MASTER_ICW);
						}
					break;

				case 'u':
				case 'm':

					switch (porttype) {
					case 'm':
						Jdb::pic_status |= (1 << buf.irq.irq);
						printf(" - PIC mask will be modified on \"g\"\n");
						break;
					case 'u':
						Jdb::pic_status  &= ~(Unsigned16)(1 << buf.irq.irq);
						printf(" - PIC mask will be modified on \"g\"\n");
						break;
					}
					break;

				default:
					printf("%c - unknown port type (must be 1,2,4 or p)\n", porttype);
					return NOTHING;
				}
    }
  return NOTHING;
}

PUBLIC
int const Io_m::num_cmds() const
{ 
  return 2;
}

PUBLIC
Jdb_module::Cmd const *const Io_m::cmds() const
{
  static Cmd cs[] =
    { Cmd( 0, "i", "in",  " type: %c", 
					 "i{1|2|4|p}xxx\tin port",  (void*)&porttype ),
      Cmd( 1, "o", "out", " type: %c", 
					 "o{1|2|4|a|u|m}xxx\tout port, ack/(un)mask/ack irq", (void*)&porttype ),
    };

  return cs;
}

PUBLIC
Io_m::Io_m()
	: Jdb_module("INFO")
{}
