/*
 * Fiasco ia32-native
 * Architecture specific IDT code
 */

IMPLEMENTATION[ia32]:

#include <flux/x86/seg.h>

IMPLEMENT
void
Idt::load_idt (pseudo_descriptor *desc)
{
  asm volatile ("lidt %0" : : "m" (desc->limit));
}
