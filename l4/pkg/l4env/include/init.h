/*!
 * \file   l4env/include/init.h
 * \brief  Initcall handling for l4env
 *
 * \date   11/27/2002
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */

#ifndef __L4ENV_INCLUDE_INIT_H_
#define __L4ENV_INCLUDE_INIT_H_

#include <l4/env/cdefs.h>

__BEGIN_DECLS

typedef void (*l4env_initcall_t)(void);

extern l4env_initcall_t __l4env_initcall_start, __l4env_initcall_end;

#define l4env_initcall(fn)	\
	static l4env_initcall_t __l4env_initcall_##fn __l4env_init_call = fn

#define __l4env_init_call 	\
	__attribute__ ((unused, __section__ (".l4env_initcall.init")))

/*!\brief Call the registered initcall functions
 *
 * This function calls all functions that registered using the
 * l4env_initcall macro. Do not forget to link using the main_stat.ld linker
 * script that respects the ".l4env_inicall.init" section.
 */
extern void l4env_do_initcalls(void);

__END_DECLS
#endif
