#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/util/idt.h>

#include <stdlib.h>

#ifdef DEBUG
    int _DEBUG = 1;
#else
    int _DEBUG = 0;
#endif
/****************************************************************
 *
 * This application demonstrates a difference occuring between
 * CPU exceptions on Fiasco/Native and FiascoUX. The application
 * sets up a local IDT and registers handlers for
 *
 * 1. DIV0 - CPU exception signalling integer division by zero
 * 2. FPE  - CPU exception signalling a floating point exception
 *
 * Afterwards the app divides an integer by zero. Native Fiasco
 * will correctly run into the handler routine for DIV0 while
 * FiascoUX will tell about an FPE.
 *
 * The reason for that is that UX emulates the CPU exceptions by
 * signals it receives from the underlying Linux. Linux however
 * sends out SIGFPE on every mathematical exception (DIV0, FPE,
 * SIMD) and UX does not separate these again.
 * 
 ***************************************************************/

typedef struct exception_state
{
    l4_umword_t edi;
    l4_umword_t esi;
    l4_umword_t ebp;
    l4_umword_t esp;
    l4_umword_t ebx;
    l4_umword_t edx;
    l4_umword_t ecx;
    l4_umword_t eax;

    l4_umword_t addr;
    l4_umword_t error;
    l4_umword_t eip;
    l4_umword_t cs;
    l4_umword_t eflags;
} exc_state_t;

extern void fpe_entry(); 
extern void div_entry();

#define DIV_EXC_NO   0
#define FPE_EXC_NO  16
#define NO_IDT_ENTRIES  (FPE_EXC_NO + 2)
#define IDT_SIZE        (sizeof(l4util_idt_header_t) + NO_IDT_ENTRIES \
                        * sizeof(l4util_idt_desc_t))

void fpe_handler(exc_state_t *state)
{
    LOG("Floating point exception occured");
    LOG("Address: %lx", state->addr);
    LOG("EIP: %lx", state->eip);

    l4_sleep_forever();
}
    
void div_handler(exc_state_t *state)
{
    LOG("Integer division by zero detected!");
    LOG("Address: %lx", state->addr);
    LOG("EIP: %lx", state->eip);

    l4_sleep_forever();
}

int main(int argc, char **argv)
{
    volatile int i=16;

    unsigned char idt_buf[IDT_SIZE];
    l4util_idt_header_t *idt = (l4util_idt_header_t *)idt_buf;
    
    // set up IDT
    l4util_idt_init(idt, NO_IDT_ENTRIES);
    l4util_idt_entry(idt, DIV_EXC_NO, (void *)div_entry);
    l4util_idt_entry(idt, FPE_EXC_NO, (void *)fpe_entry);
    l4util_idt_load(idt);
    LOG("loaded idt");

    l4_sleep(1000);
    LOG("producing exception, i = %d", i);
    // produce floating point exception
    i /= 0;

    l4_sleep_forever();
    return 0;
}
