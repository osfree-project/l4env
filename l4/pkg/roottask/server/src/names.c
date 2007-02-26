#include "stdio.h"
#include "string.h"
#include "names.h"

/* rmgr name resolution service for started tasks  */

static mb_mod_name_t module_names[MODS_MAX] = { {0} };
static l4_threadid_t module_ids[MODS_MAX]   = { L4_INVALID_ID, };
static unsigned      max_modules_names;

void
names_set(l4_threadid_t task, const char *name)
{
  module_ids[max_modules_names] = task;
  snprintf(module_names[max_modules_names],
	   sizeof(module_names[0]), "%s", name);
  max_modules_names++;
}

l4_threadid_t
names_get_id(const char *name)
{
  unsigned int i;
  char *ptr, *end;

  for (i = 0; i < max_modules_names; i++)
    {
      if (module_names[i][0] == '\0')
	continue;

      for (ptr = end = module_names[i]; *end!='\0' && *end!=' '; end++)
	if (*end == '/')
	  ptr = end + 1;

      if (!strncmp (ptr, name, end - ptr))
	return module_ids[i];
    }

  return L4_INVALID_ID;
}
