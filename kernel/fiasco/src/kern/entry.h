// -*- c++ -*-

#ifndef ENTRY_H
#define ENTRY_H

#include <flux/x86/base_trap.h>

extern "C" void profile_interrupt_entry();

extern gate_init_entry idt_init_table[];

const unsigned idt_max = 0x40;	// 0x20 CPU excps, 0x10 irqs, 7 syscalls
				// 0x3e/0x3f for APIC exceptions

#endif
