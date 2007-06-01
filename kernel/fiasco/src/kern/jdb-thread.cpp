INTERFACE:

class Space;

EXTENSION class Jdb
{
public:
  /**
   * Deliver Thread object which was active at entry of kernel debugger.
   * If we came from the kernel itself, return Thread with id 0.0
   */
  static Thread *get_thread();
};

//---------------------------------------------------------------------------
IMPLEMENTATION:

#include "jdb_prompt_ext.h"
#include "jdb_thread_names.h"
#include "jdb.h"
#include "thread.h"

PUBLIC
static int
Jdb::is_valid_task(Task_num task)
{
  return task == Config::kernel_taskno || lookup_space(task) != 0;
}

class Jdb_tid_ext : public Jdb_prompt_ext
{
public:
  void ext();
  void update();
};

IMPLEMENT
void
Jdb_tid_ext::ext()
{
  if (Jdb::get_current_active())
    printf("(%x.%02x) ",
	Jdb::get_current_active()->d_taskno(),
	Jdb::get_current_active()->d_threadno());
}

IMPLEMENT
void
Jdb_tid_ext::update()
{
  Jdb::get_current();
}

static Jdb_tid_ext jdb_tid_ext INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

//-

class Jdb_thread_name_ext : public Jdb_prompt_ext
{
public:
  void ext();
  void update();
};

IMPLEMENT
void
Jdb_thread_name_ext::ext()
{
  if (Jdb::get_current_active())
    {
      const char *s = Jdb_thread_names::lookup(Jdb::get_current_active()->id(), 1)->name();
      if (s && s[0])
        printf("[%s] ", s);
    }
}

IMPLEMENT
void
Jdb_thread_name_ext::update()
{
  Jdb::get_current();
}

static Jdb_thread_name_ext jdb_thread_name_ext INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

#include "space_index.h"
#include "kernel_task.h"

PUBLIC static
Space*
Jdb::lookup_space(Task_num task)
{
  return task == Config::kernel_taskno
    			? Kernel_task::kernel_task()
			: Space_index(task).lookup();
}

//---------------------------------------------------------------------------
IMPLEMENTATION [!{ia32,amd64}]:

#include "mem_layout.h"

IMPLEMENT
Thread *
Jdb::get_thread()
{
  Context *c = current_context();

  return (Mem_layout::in_tcbs((Address)c))
    ? reinterpret_cast<Thread*>(c)
    : reinterpret_cast<Thread*>(Mem_layout::Tcbs);
}

//---------------------------------------------------------------------------
IMPLEMENTATION:

#include "jdb.h"
#include "thread.h"

PUBLIC
static void
Jdb::get_current()
{
  current_active = get_thread();
  if (!current_active->is_mapped() ||
      current_active->id().task() != current_active->task_calculated() ||
      current_active->id().lthread() != current_active->lthread_calculated())
    current_active = 0;
}

PUBLIC static inline NEEDS["thread.h"]
Space*
Jdb::get_current_space()
{
  return current_active ? current_active->space() : 0;
}

PUBLIC static inline NEEDS["thread.h"]
Task_num
Jdb::get_current_task()
{
  return current_active ? current_active->id().task() : 0;
}
