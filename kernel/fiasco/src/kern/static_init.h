/* -*- c++ -*- */

#include "initcalls.h"

/**
 * definitions for static, pre-main initialization suff
 */
#ifndef STATIC_INIT_H
#define STATIC_INIT_H

/// Priorities for ordered static initialization
//@{

#define	DEPENDS_ON(SUBSYS)	((SUBSYS) + 2)

#define LINKER_INTERNAL		100		// linker internal use
#define MAX_INIT_PRIO		DEPENDS_ON(LINKER_INTERNAL)
#define STARTUP_INIT_PRIO       DEPENDS_ON(MAX_INIT_PRIO)
#define PERF_CNT_INIT_PRIO	DEPENDS_ON(STARTUP_INIT_PRIO)
#define JDB_CATEGORY_INIT_PRIO	DEPENDS_ON(STARTUP_INIT_PRIO)
#define JDB_MODULE_INIT_PRIO	DEPENDS_ON(JDB_CATEGORY_INIT_PRIO)

#define UART_INIT_PRIO		DEPENDS_ON(STARTUP_INIT_PRIO)
#define KDB_INIT_PRIO		DEPENDS_ON(JDB_MODULE_INIT_PRIO)
#define JDB_INIT_PRIO		DEPENDS_ON(KDB_INIT_PRIO)

#define INIT_PRIORITY(a) __attribute__((init_priority(a)))

//@}



/* ATTENTION: WARNING: DON'T READ ANY FURTHER !!! */
/*  The following definitions are likely to cause */
/*  extreme and irreparable BRAIN DAMAGE.         */
/*  (I already tested this on my unsuspecting    */
/*   colleagues)                                  */

/**
 * \defgroup Magic static initializer macros
 */
//@{
#define __STATIC_INITIALIZER(_func, va) \
class static_init_##va { \
public: \
  static_init_##va() FIASCO_INIT; \
};\
static_init_##va::static_init_##va() { \
  _func(); \
} \
static static_init_##va __static_construction_of_##va##__

#define __STATIC_INITIALIZER_P(_func, va, prio) \
class static_init_##va { \
public: \
  static_init_##va() FIASCO_INIT; \
};\
static_init_##va::static_init_##va() { \
  _func(); \
} \
static static_init_##va __static_construction_of_##va##__ \
  __attribute__((init_priority(prio)))
//@}



/**
 * \defgroup Macros to define static init functions
 */
//@{

/// mark f as static initializer with priority p
#define STATIC_INITIALIZER_P(f, p) \
  __STATIC_INITIALIZER_P(f,func_##f,p)

/// mark f as static initailizer 
#define STATIC_INITIALIZER(f) \
  __STATIC_INITIALIZER(f,func_##f)
//@}


/// static initialization of singleton (static) classes
/**
 * The classes that should be initialized must provide 
 * a init() member function that takes no arguments.
 */
//@{

/** mark class c to be statically initialized via its init 
 *  function and with priority p
 */
#define STATIC_INITIALIZE_P(c,p) \
  __STATIC_INITIALIZER_P(c::init, class_##c,p)

/// mark class c to be statically initialized via its init function
#define STATIC_INITIALIZE(c) \
  __STATIC_INITIALIZER(c::init, class_##c)
//@}



#endif // __STATIC_INIT_H__
