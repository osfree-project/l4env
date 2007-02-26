#ifndef __DISASM_H
#define __DISASM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int         (*Peek_task) (Address addr, unsigned task,
				  Mword *value, int width);
typedef const char* (*Get_symbol) (Address addr, unsigned task);

extern unsigned int
disasm_bytes(char *buffer, unsigned len, Address addr, 
	     unsigned task, int show_symbols, int show_intel_syntax,
	     Peek_task peek_task, Get_symbol get_symbol);

#ifdef __cplusplus
}
#endif

#endif

