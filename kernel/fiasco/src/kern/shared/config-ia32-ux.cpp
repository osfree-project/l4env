INTERFACE:

EXTENSION class Config
{
public:

  enum {
    PAGE_SHIFT = 12,
    PAGE_SIZE  = 1 << PAGE_SHIFT,
    PAGE_MASK  = ~( PAGE_SIZE - 1),

    SUPERPAGE_SHIFT = 22,
    SUPERPAGE_SIZE  = 1 << SUPERPAGE_SHIFT,
    SUPERPAGE_MASK  = ~( SUPERPAGE_SIZE -1 ),

    MAX_NUM_IRQ = 20,
  };

  enum {
    AUTO_MAP_KIP_ADDRESS = 0xbfff0000,
  };

};


IMPLEMENTATION[ia32-ux]:
//-

