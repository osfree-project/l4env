INTERFACE[ia32,ux]:

EXTENSION class Config
{
public:

  enum {
    PAGE_SHIFT          = 12,
    PAGE_SIZE           = 1 << PAGE_SHIFT,
    PAGE_MASK           = ~( PAGE_SIZE - 1),

    SUPERPAGE_SHIFT     = 22,
    SUPERPAGE_SIZE      = 1 << SUPERPAGE_SHIFT,
    SUPERPAGE_MASK      = ~( SUPERPAGE_SIZE -1 ),

    Max_num_irqs        = 20,
    Max_num_dirqs       = 16,
    Irq_shortcut        = 1,
  };

  enum {
    SCHED_PIT		= 0,
    SCHED_RTC		= 1,
    SCHED_APIC		= 2,
  };
};
