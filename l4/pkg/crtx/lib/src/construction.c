#include <l4/sys/l4int.h>
#include <l4/sys/compiler.h>
#include <l4/sys/kdebug.h>
#include <l4/crtx/crt0.h>

// #define DEBUG

int __cxa_atexit(void (*function)(void *), void *arg, void *dso_handle);

#define BEG		{ (crt0_hook) ~1U }
#define END		{ (crt0_hook)   0 }

// make sure that unused symbols are not discarded
#if (__GNUC__ == 3 && __GNUC_MINOR__ >= 3) || __GNUC__ >= 4
#define SECTION(x)	__attribute__((used, section( x )))
#else
#define SECTION(x)	__attribute__((section( x )))
#endif

typedef void (*const crt0_hook)(void);

static crt0_hook __L4DDE_CTOR_BEG__[1] SECTION(".mark_beg_l4dde_ctors")  = BEG;
static crt0_hook __L4DDE_CTOR_END__[1] SECTION(".mark_end_l4dde_ctors")  = END;
static crt0_hook __CTOR_BEG__[1]       SECTION(".mark_beg_ctors")        = BEG;
static crt0_hook __CTOR_END__[1]       SECTION(".mark_end_ctors")        = END;
static crt0_hook __C_CTOR_BEG__[1]     SECTION(".mark_beg_c_ctors")      = BEG;
static crt0_hook __C_CTOR_END__[1]     SECTION(".mark_end_c_ctors")      = END;
static crt0_hook __DTOR_BEG__[1]       SECTION(".mark_beg_dtors")        = BEG;
static crt0_hook __DTOR_END__[1]       SECTION(".mark_end_dtors")        = END;
static crt0_hook __C_SYS_DTOR_BEG__[1] SECTION(".mark_beg_c_sys_dtors")  = BEG;
static crt0_hook __C_SYS_DTOR_END__[1] SECTION(".mark_end_c_sys_dtors")  = END;
static crt0_hook __C_DTOR_BEG__[1]     SECTION(".mark_beg_c_dtors")      = BEG;
static crt0_hook __C_DTOR_END__[1]     SECTION(".mark_end_c_dtors")      = END;


static void
run_hooks_forward(crt0_hook list[], const char *name)
{
#ifdef DEBUG
  outstring("list (forward) ");
  outstring(name);
  outstring(" @ ");
  outhex32((unsigned)list);
  outchar('\n');
#endif
  list++;
  while (*list)
    {
#ifdef DEBUG
      outstring("  calling ");
      outhex32((unsigned)*list);
      outchar('\n');
#endif
      (**list)();
      list++;
    }
}

static void
run_hooks_backward(crt0_hook list[], const char *name)
{
#ifdef DEBUG
  outstring("list (backward) ");
  outstring(name);
  outstring(" @ ");
  outhex32((unsigned)list);
  outchar('\n');
#endif
  list--;
  while (*list != (crt0_hook)~1U)
    {
#ifdef DEBUG
      outstring("  calling ");
      outhex32((unsigned)*list);
      outchar('\n');
#endif
      (**list)();
      list--;
    }
}

static void
static_construction(void)
{
  /* call constructors made with L4_C_CTOR */
  run_hooks_forward(__C_CTOR_BEG__, "__C_CTOR_BEG__");

  /* call constructors made with __attribute__((constructor))
   * and static C++ constructors */
  run_hooks_backward(__CTOR_END__, "__CTOR_END__");
}

static void
static_destruction(void *x __attribute__((unused)))
{
  /* call destructors made with __attribute__((destructor))
   * and static C++ destructors */
  run_hooks_forward(__DTOR_BEG__, "__DTOR_BEG__");

  /* call destructors made with L4_C_DTOR except system destructors */
  run_hooks_backward(__C_DTOR_END__, "__C_DTOR_END__");
}

/* call system destructors */
void
crt0_sys_destruction(void)
{
  run_hooks_forward(__C_SYS_DTOR_BEG__, "__C_SYS_DTOR_BEG__");
}

extern void *__dso_handle __attribute__((weak));

/* is called by crt0 immediately before calling __main() */
void
crt0_construction(void)
{
  static_construction();
  __cxa_atexit(&static_destruction, 0, &__dso_handle == 0 ? 0 : __dso_handle);
}

void
crt0_dde_construction(void)
{
  run_hooks_forward(__L4DDE_CTOR_BEG__, "__L4DDE_CTOR_BEG__");
}

asm (".hidden _init");

/* this special function is called for initializing static libraries */
void _init(void) __attribute__((section(".init")));
void l4sys_fixup_abs_syscalls(void) __attribute__((weak));
void
_init(void)
{
  if (l4sys_fixup_abs_syscalls)
    l4sys_fixup_abs_syscalls();
  static_construction();
}
