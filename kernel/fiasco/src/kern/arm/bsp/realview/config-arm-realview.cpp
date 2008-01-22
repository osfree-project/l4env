INTERFACE[arm && realview]:
#define TARGET_NAME "Realview"

INTERFACE [arm && realview && !mpcore]:

EXTENSION class Config
{
public:
  enum
  {
    Scheduling_irq       = 36,
    scheduler_irq_vector = Scheduling_irq,
    Max_num_irqs         = 98,
    Max_num_dirqs        = 96,

    Vkey_irq             = 96,
    Tbuf_irq             = 97,
  };
};

INTERFACE [arm && realview && mpcore]:

EXTENSION class Config
{
public:
  enum
  {
    Scheduling_irq       = 33,
    scheduler_irq_vector = Scheduling_irq,
    Max_num_irqs         = 66,
    Max_num_dirqs        = 64,

    Vkey_irq             = 64,
    Tbuf_irq             = 65,
  };
};
