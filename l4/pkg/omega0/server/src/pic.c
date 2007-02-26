#include <l4/util/irq.h>
#include <l4/log/l4log.h>
#include "config.h"
#if OMEGA0_USE_PIC_LOCKING
#include "lock.h"
#endif
#include "globals.h"
#include "pic.h"

#if OMEGA0_USE_PIC_LOCKING
/* locking stuff */
static wq_lock_queue_base pic_wq = {NULL};
#define declare_wqe	wq_lock_queue_elem wqe
#define LOCK		wq_lock_lock(&pic_wq, &wqe)
#define UNLOCK		wq_lock_unlock(&pic_wq, &wqe)
#else
#define declare_wqe	/* */
#define LOCK		/* */
#define UNLOCK		/* */
#endif

int use_special_fully_nested_mode = 1;

void irq_mask(int irq){
    declare_wqe;

    LOGdl(OMEGA0_DEBUG_PIC, "(%x) called", irq);

    LOCK;
    l4util_cli();
    if(irq<8){
	l4util_out8(l4util_in8(0x21) | (1<<irq), 0x21);  
    } else {
	l4util_out8(l4util_in8(0xa1) | (1<<(irq-8)), 0xa1);  
    }
    l4util_sti();
    UNLOCK;
}

void irq_unmask(int irq){
    declare_wqe;

    LOGdl(OMEGA0_DEBUG_PIC, "(%x) called", irq);

    LOCK;  
    l4util_cli();
    if(irq<8){
	l4util_out8(l4util_in8(0x21) & ~(1<<irq), 0x21);  
    } else {
	l4util_out8(l4util_in8(0xa1) & ~(1<<(irq-8)), 0xa1);
    }
    l4util_sti();
    UNLOCK;
}

void irq_ack(int irq){
    declare_wqe;

    LOGdl(OMEGA0_DEBUG_PIC, "(%x) called", irq);

    LOCK;
    l4util_cli();
#if OMEGA0_STRATEGY_SPECIFIC_EOI
    if (irq > 7){
	l4util_out8(0x60|(irq&7),0xA0);
	if (use_special_fully_nested_mode){
	    l4util_out8(0x0B,0xA0);
	}
	if (!use_special_fully_nested_mode || l4util_in8(0xA0) == 0){
	    l4util_out8(0x62,0x20);
	}
    }else{
	l4util_out8(0x60|irq,0x20);
    }
#else
    if (irq > 7){
	l4util_out8(0x20,0xA0);
	if (use_special_fully_nested_mode){
	    l4util_out8(0x0B,0xA0);
	}
	if (!use_special_fully_nested_mode || l4util_in8(0xA0) == 0){
	    l4util_out8(0x20,0x20);
	}
    }else{
	l4util_out8(0x20,0x20);
    }
#endif
    l4util_sti();
    UNLOCK;
}


void irq_mask_and_ack(int irq){
    declare_wqe;

    LOGdl(OMEGA0_DEBUG_PIC, "(%x) called", irq);

    LOCK;
    l4util_cli();
    if(irq<8){
	l4util_out8(l4util_in8(0x21) | (1<<irq), 0x21);  
#if OMEGA0_STRATEGY_SPECIFIC_EOI
	l4util_out8(0x60|irq,0x20);
#else
	l4util_out8(0x20,0x20);
#endif
    } else {
	l4util_out8(l4util_in8(0xa1) | (1<<(irq-8)), 0xa1);  
#if OMEGA0_STRATEGY_SPECIFIC_EOI
	l4util_out8(0x60|(irq&7),0xA0);
	if (use_special_fully_nested_mode){
	    l4util_out8(0x0B,0xA0);
	}
	if (!use_special_fully_nested_mode || l4util_in8(0xA0) == 0){
	    l4util_out8(0x62, 0x20);
	}
#else
	l4util_out8(0x20, 0xA0);
	if (use_special_fully_nested_mode){
	    l4util_out8(0x0B, 0xA0);
	}
	if (!use_special_fully_nested_mode || l4util_in8(0xA0) == 0){
	    l4util_out8(0x20, 0x20);
	}
#endif
    }
    l4util_sti();
    UNLOCK;
}


/*!\brief Value of the interrupt service register (isr).
 *
 * ISR holds the irq's which are accepted by the processor and not
 * acknowledged.
 *
 * \param master if !=0, return master ISR, else return slave ISR */
int pic_isr(int master){
    int dat;
    declare_wqe;

    LOCK;
    l4util_cli();
    if (master){
	l4util_out8(0xb,0xA0);
	dat = l4util_in8(0xa0);
    }else{
	l4util_out8(0xb,0x20);
	dat = l4util_in8(0x20);
    }
    l4util_sti();
    UNLOCK;
    return dat;
}

/*!\brief Value of the interrupt request register (IRR).
 *
 * IRR holds the irq's which are requested by hardware but not
 * delivered to the processor.
 *
 * \param master if !=0, return master IRR, else return slave IRR */
int pic_irr(int master){
    int dat;
    declare_wqe;

    LOCK;
    l4util_cli();
    if (master){
	l4util_out8(0xa,0xA0);
	dat = l4util_in8(0xa0);
    }else{
	l4util_out8(0xa,0x20);
	dat = l4util_in8(0x20);
    }
    l4util_sti();
    UNLOCK;
    return dat;
}

/*!\brief Value of the interrupt mask register (IMR).
 *
 * \param master if !=0, return master IMR, else return slave IMR */
int pic_imr(int master){
    int dat;
    declare_wqe;

    LOCK;
    l4util_cli();
    if (master){
	dat = l4util_in8(0xa1);
    }else{
	dat = l4util_in8(0x21);
    }
    l4util_sti();
    UNLOCK;
    return dat;
}
