#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/signal/signal-client.h>
#include <l4/util/idt.h>
#include <l4/util/thread.h>
#include <l4/log/l4log.h>

#include "exceptions.h"

#include "local.h"

extern void l4signal_segv_entry(void);
extern void l4signal_fpe_entry(void);
extern void l4signal_illegal_entry(void);
extern void l4signal_stackf_entry(void);

/**********************************/
/* The CPU interrupts of interest */
/**********************************/
// division by zero --> fpe
#define DIV_VECTOR  0
// overflow --> fpe
#define OF_VECTOR   4
// illegal opcode --> SIGILL
#define ILL_VECTOR  6
// double fault --> segmentation fault
#define DOUBLE_VECTOR 8
// stack fault --> SIGSTKFLT
#define STACK_VECTOR 12
// general protection fault --> segmentation fault
#define GP_VECTOR 13
// floating point exception --> fpe
#define FPE_VECTOR 16
// floating point exception on SIMD --> fpe
#define SIMD_FPE_VECTOR 19

#define NO_IDT_ENTRIES 21
#define IDT_SIZE    (sizeof(l4util_idt_header_t) + \
                    NO_IDT_ENTRIES * sizeof(l4util_idt_desc_t))

extern l4_threadid_t l4signal_signal_server_id;

// this static buffer is used to hold our copy of the 
// IDT. Even though each thread needs to set up his own
// IDT, we do not need more than one copy of it as we 
// fill in the same values every time
static unsigned char l4signal_idt_buf[IDT_SIZE];
static l4util_idt_header_t *l4signal_idt = NULL;

void l4signal_init_idt(void)
{
    if (l4signal_idt == NULL)
    {
        l4signal_idt = (l4util_idt_header_t *)l4signal_idt_buf;
    
        l4util_idt_init(l4signal_idt, NO_IDT_ENTRIES);

        l4util_idt_entry(l4signal_idt, DIV_VECTOR,       (void *)l4signal_fpe_entry);
        l4util_idt_entry(l4signal_idt, OF_VECTOR,        (void *)l4signal_fpe_entry);
        l4util_idt_entry(l4signal_idt, ILL_VECTOR,       (void *)l4signal_illegal_entry);
        l4util_idt_entry(l4signal_idt, DOUBLE_VECTOR,    (void *)l4signal_segv_entry);
        l4util_idt_entry(l4signal_idt, STACK_VECTOR,     (void *)l4signal_stackf_entry);
        l4util_idt_entry(l4signal_idt, GP_VECTOR,        (void *)l4signal_segv_entry);
        l4util_idt_entry(l4signal_idt, FPE_VECTOR,       (void *)l4signal_fpe_entry);
        l4util_idt_entry(l4signal_idt, SIMD_FPE_VECTOR,  (void *)l4signal_fpe_entry);
    }

    l4util_idt_load(l4signal_idt);
//    LOG("idt initialized, DIV0 = %x", idt->desc[DIV_VECTOR].a);
//    LOG("idt initialized, OVERFLOW = %x", idt->desc[OF_VECTOR].a);
//    LOG("idt initialized, ILL = %x", idt->desc[ILL_VECTOR].a);
//    LOG("idt initialized, DOUBLE = %x", idt->desc[DOUBLE_VECTOR].a);
//    LOG("idt initialized, STACK = %x", idt->desc[STACK_VECTOR].a);
//    LOG("idt initialized, GPF = %x", idt->desc[GP_VECTOR].a);
//    LOG("idt initialized, FPE = %x", idt->desc[FPE_VECTOR].a);
//    LOG("idt initialized, SIMDFPE = %x", idt->desc[SIMD_FPE_VECTOR].a);
//    LOG("l4signal_fpe_entry = %x", l4signal_fpe_entry);
//    LOG("l4signal_segv_entry = %x", l4signal_segv_entry);
//    LOG("l4signal_illegal_entry = %x", l4signal_illegal_entry);
}

void l4signal_segv_handler(exception_state *exc)
{
    siginfo_t siginfo;
    l4_threadid_t me = l4_myself();
    int i;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc  = (dice_malloc_func)malloc;
    _dice_corba_env.free    = (dice_free_func)free;
    
    
    siginfo.si_signo = SIGSEGV;
    siginfo.si_addr = (void *)exc->addr;

    i = signal_signal_kill_call(&l4signal_signal_server_id, &me,
            &siginfo, &_dice_corba_env);
}

void l4signal_fpe_handler(exception_state *exc)
{
    siginfo_t siginfo;
    l4_threadid_t me = l4_myself();
    
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc  = (dice_malloc_func)malloc;
    _dice_corba_env.free    = (dice_free_func)free;
    LOG("FPE_handler");
    
    siginfo.si_signo = SIGFPE;
    siginfo.si_addr = (void *)exc->addr;

    signal_signal_kill_call(&l4signal_signal_server_id, &me,
            &siginfo, &_dice_corba_env);
}

void l4signal_illegal_handler(exception_state *exc)
{
    siginfo_t siginfo;
    l4_threadid_t me = l4_myself();
    
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc  = (dice_malloc_func)malloc;
    _dice_corba_env.free    = (dice_free_func)free;
    
    siginfo.si_signo = SIGILL;
    siginfo.si_addr = (void *)exc->addr;

    signal_signal_kill_call(&l4signal_signal_server_id, &me,
            &siginfo, &_dice_corba_env);
}

void l4signal_stackf_handler(exception_state *exc)
{
    siginfo_t siginfo;
    l4_threadid_t me = l4_myself();
    
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc  = (dice_malloc_func)malloc;
    _dice_corba_env.free    = (dice_free_func)free;
    
    siginfo.si_signo = SIGSTKFLT;
    siginfo.si_addr = (void *)exc->addr;

    signal_signal_kill_call(&l4signal_signal_server_id, &me,
            &siginfo, &_dice_corba_env);
}
