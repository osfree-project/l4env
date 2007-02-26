#include <stdio.h>
#include <l4/petze/petze.h>
#include "local.h"

void *__wrap_malloc(unsigned int size)
{
	char buf[22];
	snprintf(buf, sizeof(buf), "[%p]", __builtin_return_address(0));
	return petz_malloc(buf, size);
}

void *__wrap_calloc(unsigned int nmemb, unsigned int size)
{
	char buf[22];
	snprintf(buf, sizeof(buf), "[%p]", __builtin_return_address(0));
	return petz_calloc(buf, nmemb, size);
}

void *__wrap_realloc(void *addr, unsigned int size)
{
	char buf[22];
	snprintf(buf, sizeof(buf), "[%p]", __builtin_return_address(0));
	return petz_realloc(buf, addr, size);
}

void  __wrap_free(void *addr)
{
	char buf[22];
	snprintf(buf, sizeof(buf), "[%p]", __builtin_return_address(0));
	petz_free(buf, addr);
}


/*** WRAPPER FOR NEW() C++ OPERATOR ***/
void *WRAP_CXX_NEW(unsigned int size)
{
	char buf[30];
	void *addr;
	snprintf(buf, sizeof(buf), "[new,EIP=0x%p]",
	         __builtin_return_address(0));
	addr = petz_cxx_new(buf, size);
	return addr;
}


/*** WRAPPER FOR DELETE() C++ OPERATOR ***/
void  WRAP_CXX_FREE(void *addr)
{
	char buf[35];
	snprintf(buf, sizeof(buf), "[delete,EIP=0x%p]",
	         __builtin_return_address(0));
	petz_cxx_free(buf, addr);
}

