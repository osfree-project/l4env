#include <stdio.h>
#include <l4/names/libnames.h>
#include <l4/loader/loader-client.h>
#include <l4/env/env.h>
#include <l4/util/parse_cmd.h>
#include <l4/loader/loader.h>

char LOG_tag[9] = "dynamic";

static l4_threadid_t loader_id;
static l4_threadid_t fprov_id;
static envpage_t envpage;

extern "C" void bar_in_binary(void);

unsigned (*dyn_add)(unsigned a, unsigned b);
void     (*dyn_show)(void);

static void
show_psecs(l4env_infopage_t *env)
{
  int i;

  for (i=0; i<env->section_num; i++)
    {
      printf("  #%02x: %08x-%08x  flags %04x\n",
	     i,
	     env->section[i].addr,
	     env->section[i].addr+env->section[i].size,
	     env->section[i].info.type);
    }
}

void
bar_in_binary(void)
{
  printf("This is bar_in_binary (called from the foo library)\n");
}

int
main(int argc, const char *argv[])
{
  DICE_DECLARE_ENV(_env);
  l4env_infopage_t *env;
  int error;
  l4_addr_t addr;
  const char *fprov_name;

  if ((error = parse_cmdline(&argc, &argv,
		'f', "fprov", "specify file provider",
		PARSE_CMD_STRING, "TFTP", &fprov_name,
		0)))
    {
      switch (error)
	{
	case -1: printf("Bad parameter for parse_cmdline()\n"); break;
	case -2: printf("Out of memory in parse_cmdline()\n"); break;
	default: printf("Error %d in parse_cmdline()\n", error); break;
	}
      return 1;
    }
  printf("\n"
         "Testing loading of dynamic libraries at runtime\n"
         "===============================================\n"
	 "\n");

  if (!names_waitfor_name("LOADER", &loader_id, 5000))
    {
      printf("Dynamic loader LOADER not found\n");
      return -1;
    }
  if (!names_waitfor_name(fprov_name, &fprov_id, 5000))
    {
      printf("File provider %s not found\n", fprov_name);
      return -2;
    }
  if ((error = l4loader_app_lib_open_call(&loader_id, "libfoo_dyn_c++.s.so",
					  &fprov_id, 0, &envpage, &_env))
      || _env.major != CORBA_NO_EXCEPTION)
    {
      printf("Loading lib failed (error %d, exc=%d.%d)\n",
	  error, _env.major, _env.repos_id);
      return -3;
    }

  printf("Lib successfully loaded!\n");
  env = (l4env_infopage_t*)envpage;

  // Show all program sections. There should be 6 sections now: dyn_test.code,
  // dyn_test.data, libloader.s.so.code, libloader.s.so.data, libfoo_dyn.code,
  // and libfoo_dyn.data
  puts("Before relocating (note the last two sections not yet relocated):");
  show_psecs(env);

  // Attach all program sections to our region mapper. This function is
  // implemented in libloader.s.so
  l4loader_attach_relocateable(env);

  puts("After relocating (note the last two sections beeing relocated now):");
  show_psecs(env);

  // Now go back to the loader and ask him to link the library.
  if ((error = l4loader_app_lib_link_call(&loader_id, &envpage, &_env))
      || _env.major != CORBA_NO_EXCEPTION)
    {
      printf("Linking lib failed (error %d, exc=%d.%d)\n",
	  error, _env.major, _env.repos_id);
      return -4;
    }

  // try to call foo_add
  if ((error = l4loader_app_lib_dsym_call(&loader_id, "foo_add",
					  &envpage, &addr, &_env))
      || _env.major != CORBA_NO_EXCEPTION)
    {
      printf("Error searching \"foo_add\" (error %d, exc=%d.%d)\n",
	  error, _env.major, _env.repos_id);
      return -5;
    }
  printf("Found symbol \"foo_add\" at address %08x\n", addr);

  dyn_add = (unsigned (*)(unsigned a, unsigned b))addr;
  printf("==> foo_add(%d, %d) => %d\n", 4, 5, dyn_add(4, 5));

  // try to call foo_show
  if ((error = l4loader_app_lib_dsym_call(&loader_id, "foo_show",
					  &envpage, &addr, &_env))
      || _env.major != CORBA_NO_EXCEPTION)
    {
      printf("Error searching \"foo_show\" (error %d, exc=%d.%d)\n",
	  error, _env.major, _env.repos_id);
      return -5;
    }
  printf("Found symbol \"foo_show\" at address %08x\n", addr);

  dyn_show = (void (*)(void))addr;
  printf("==> foo_show()\n");
  dyn_show();

  printf("Well done!\n");

  return 0;
}
