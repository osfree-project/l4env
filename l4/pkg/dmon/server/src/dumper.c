/**
 * \file   server/src/dumper.c
 * \brief  Dmon server statistics dumper
 *
 * \date   10/01/2004
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 */
#include <l4/util/macros.h>
#include <l4/env/env.h>
#include <l4/log/l4log.h>
#include <l4/dope/dopelib.h>
#include <l4/dope/dopedef.h>

/* standard */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef WITH_TMPFS
#define WITH_TMPFS 0
#endif

#ifndef WITH_PETZE
#define WITH_PETZE 0
#endif

/* support these servers */
#include <l4/rmgr/librmgr.h>
#include <l4/names/libnames.h>
#include <l4/events/events.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/loader/loader-client.h>
#if WITH_PETZE
#include <l4/petze/petze-client.h>
#endif
#if WITH_TMPFS
#include <l4/tmpfs/tmpfs.h>
#include <l4/tmpfs/tmpfs-client.h>
#endif
#include <l4/dm_generic/dm_generic.h>
#include <l4/dm_phys/dm_phys.h>

/* local */
#include "local.h"

/*** LIST ALL BUTTONS HERE ***/
enum {
	DUMP_RMGR,
	DUMP_NAMES,
	DUMP_EVENTS,
	DUMP_TS,
	DUMP_LOADER,
	DUMP_PETZE,
	DUMP_TMPFS,
	DUMP_DM
};

/*** MODULE VARIABLES ***/
static long app_id;
static int loader_try;
static l4_threadid_t loader_id = L4_INVALID_ID;
#if WITH_PETZE
static int petze_try;
static l4_threadid_t petze_id = L4_INVALID_ID;
#endif
#if WITH_TMPFS
static int tmpfs_try;
static l4_threadid_t tmpfs_id = L4_INVALID_ID;
#endif

/*** TOOL: PARSE STRING FOR L4 V2 THREADID ***
 *
 * Only valid format is "task.thread". task and thread are hex numbers.
 * Wildcard "*" is allowed and "-1" is returned for this ID.
 *
 * Return !0 on parse error.
 */
static int parse_v2_threadid(const char *id, unsigned *task, unsigned *thread)
{
	int ret;
	char tmp[64];
	char *p;

	unsigned taskid;
	unsigned threadid;
	char *taskid_p, *threadid_p;

	/* check args */
	if (!id || !task || !thread)
		return 1;

	if (strlen(id) > sizeof(tmp) - 1) {
		LOG("argument string too long");
		return 2;
	}
	strcpy(tmp, id);

	p = strchr(tmp, '.');
	if (!p)
		goto error;

	*p = 0;
	p++;
	taskid_p = tmp;
	threadid_p = p;

	/* TASKID */
	taskid = strtoul(taskid_p, &p, 16);
	if (p == taskid_p) {
		/* it's not a hex number - test for * */
		if (!(strcmp(taskid_p, "*") == 0))
			goto error;
		taskid = -1;
	}

	/* THREADID */
	threadid = strtoul(threadid_p, &p, 16);
	if (p == threadid_p) {
		/* it's not a hex number - test for * */
		if (!(strcmp(threadid_p, "*") == 0))
			goto error;
		threadid = -1;
	}

	if ((threadid != -1) && (taskid != -1))
		ret = snprintf(tmp, sizeof(tmp), "%x.%02x", taskid, threadid);
	else if ((threadid == -1) && (taskid != -1))
		ret = snprintf(tmp, sizeof(tmp), "%x.*", taskid);
	else if ((threadid != -1) && (taskid == -1))
		ret = snprintf(tmp, sizeof(tmp), "*.%02x", threadid);
	else {
		ret = 0;
		strcpy(tmp, "*.*");
	}
	if (ret >= sizeof(tmp)) {
		LOG_Error("buffer too small?");
		exit(1);
	}

	/* successful */
	*task = taskid;
	*thread = threadid;
	return 0;

error:
	/* unsuccessful */
	return 99;
}

/*** TOOL: SET/RESET STATUSLINE ***/
static void statusline(const char *msg)
{
	/* standard */
	if (!msg)
		msg = "No error";

	dope_cmdf(app_id, "dumper_l_error.set(-text \"%s\")", msg);
}

/*** TOOL: READ ENTRY CONTENTS ***/
static int read_entry(const char *entry, char *buffer, unsigned buffer_size)
{
	int err;

	err = dope_reqf(app_id, buffer, buffer_size, "%s.text", entry);
	if (err) {
		/* errors, warnings, ... */
		if (err < 0)
			LOG_Error("\"%s\" (%d) ", buffer, err);
		else if (err == DOPECMD_WARN_TRUNC_RET_STR)
			LOG("Warning: \"return string was truncated\"");
		else
			LOG("Warning: \"unknown\" %d", err);
		/* FIXME */
		return 1;
	}

	return 0;
}

/*** DUMP LOADER INFO ***
 *
 * XXX Maybe, replace parse_v2_threadid() with parse_task_number()
 */
static void dump_loader(char **msg)
{
	int err;
	char buffer[32];
	unsigned thread, task;
	CORBA_Environment env = dice_default_environment;

	err = read_entry("dumper_e_loader", buffer, sizeof(buffer));
	if (err) {
		*msg = "Internal error";
		return;
	}

	err = parse_v2_threadid(buffer, &task, &thread);
	if (err) {
		*msg = "Invalid thread ID (task.thread)";
		return;
	}

	if (task == -1)
		err = l4loader_app_dump_call(&loader_id,
		                             0, /* task ID (0 dumps all) */
		                             0, /* flags (unused in LOADER) */
		                             &env);
	else
		err = l4loader_app_dump_call(&loader_id,
		                             task,
		                             0,
		                             &env);
	if (err)
		*msg = "Error calling loader";
}

/*** DUMP DATASPACE MANGER INFO ***
 *
 * Interesting functions are:
 *
 * void l4dm_ds_show(const l4dm_dataspace_t * ds);
 * int l4dm_ds_dump(l4_threadid_t dsm_id, l4_threadid_t owner,
 *                  l4_uint32_t flags, l4dm_dataspace_t * ds);
 * int l4dm_ds_list(l4_threadid_t dsm_id, l4_threadid_t owner,
 *                  l4_uint32_t flags);
 * int l4dm_ds_list_all(l4_threadid_t dsm_id);
 *
 * void l4dm_memphys_show_memmap(void);
 * void
 * l4dm_memphys_show_pools(void);
 * void
 * l4dm_memphys_show_pool_areas(int pool);
 * void
 * l4dm_memphys_show_pool_free(int pool);
 * void
 * l4dm_memphys_show_slabs(int show_free);
 */
static void dump_dm(char **msg)
{
	int err;
	char buffer[32];
	l4_threadid_t owner = L4_NIL_ID;

	unsigned thread, task;

	err = read_entry("dumper_e_dm", buffer, sizeof(buffer));
	if (err) {
		*msg = "Internal error";
		return;
	}

	err = parse_v2_threadid(buffer, &task, &thread);
	if (err) {
		*msg = "Invalid thread ID (task.thread)";
		return;
	}

	/* FIXME This does not work because we don't know all members of
	 *       owner.id (version, ...) */
	owner.id.task = task;
	owner.id.lthread = thread;

	if ((task == -1) && (thread == -1))
		err = l4dm_ds_list_all(L4DM_DEFAULT_DSM);
	else if (thread == -1)
		err = l4dm_ds_list(L4DM_DEFAULT_DSM, owner, L4DM_SAME_TASK);
	else {
		/* FIXME err = l4dm_ds_list(L4DM_DEFAULT_DSM, owner, 0); */
		err = 0;
		*msg = "Exact thread ID not determinable";
	}

	if (err)
		*msg = "Error calling dataspace manager";
}

/*** DOpE CALLBACK: HANDLES ALL EVENTS FOR INFO DUMPING ***/
static void dump_callback(dope_event *ev, void *priv)
{
	char *msg = NULL;
	int what = (int)priv;
#if WITH_PETZE || WITH_TMPFS
	CORBA_Environment env = dice_default_environment;
#endif

	LOG_printf("===] "APP_NAME" [===================="
	           "======================================\n");

	switch (what) {
	case DUMP_RMGR:
		if (rmgr_dump_mem()) {
			msg = "Error calling RMGR";
		}
		break;

	case DUMP_NAMES:
		if (!names_dump())
			msg = "Error calling names";
		break;

	case DUMP_EVENTS:
		if (l4events_dump())
			msg = "Error calling events";
		break;

	case DUMP_TS:
		if (l4ts_dump_tasks())
			msg = "Error calling task server";
		break;

	case DUMP_LOADER:
		if (loader_try < 3) {
			names_waitfor_name("LOADER", &loader_id, 1000);
			loader_try++;
		}
		if (l4_is_invalid_id(loader_id))
			msg = "Loader not known at names";
		else
			dump_loader(&msg);
		break;

	case DUMP_PETZE:
#if WITH_PETZE
#warning compiling with PETZE support
		if (petze_try < 3) {
			names_waitfor_name("Petze", &petze_id, 1000);
			petze_try++;
		}
		if (l4_is_invalid_id(petze_id))
			msg = "Petze not known at names";
		else
			petze_dump_call(&petze_id, &env);

		/* FIXME RESET
		 *
		 * void petze_reset_call(const_CORBA_Object _dice_corba_obj,
		 *                       CORBA_Environment *_dice_corba_env);
		 */
#else
		msg = "Petze not supported by this version";
#endif
		break;

	case DUMP_TMPFS:
#if WITH_TMPFS
#warning compiling with TMPFS support
		if (tmpfs_try < 3) {
			names_waitfor_name(TMPFS_NAME, &tmpfs_id, 1000);
			tmpfs_try++;
		}
		if (l4_is_invalid_id(tmpfs_id))
			msg = "Tmpfs not known at names";
		else
			tmpfs_dump_call(&tmpfs_id, &env);
#else
		msg = "Tmpfs not supported by this version";
#endif
		break;

	case DUMP_DM:
		dump_dm(&msg);
		break;

	default:
		LOG("Unknown event: type=%ld priv=%p",
		       ev->type, priv);
	}

	statusline(msg);

	LOG_printf("=================================="
	           "==================================\n");
}

#if 0
/*** DOpE CALLBACK: HANDLES ALL EVENTS FOR X BUTTON ***/
static void exit_callback(dope_event *ev, void *priv)
{
	/* cleanup */
	dope_deinit_app(app_id);

	LOG("Goodbye..");
	exit(0);
}
#endif

/*** STATISTICS DUMPER INITIALIZATION ***/
int dmon_dumper_init(long dope_app_id)
{
	app_id = dope_app_id;

	/* register button event handlers */
	dope_bind(app_id, "dumper_b_rmgr", "commit", dump_callback, (void *)DUMP_RMGR);
	dope_bind(app_id, "dumper_b_names", "commit", dump_callback, (void *)DUMP_NAMES);
	dope_bind(app_id, "dumper_b_events", "commit", dump_callback, (void *)DUMP_EVENTS);
	dope_bind(app_id, "dumper_b_ts", "commit", dump_callback, (void *)DUMP_TS);
	dope_bind(app_id, "dumper_b_loader", "commit", dump_callback, (void *)DUMP_LOADER);
	dope_bind(app_id, "dumper_b_petze", "commit", dump_callback, (void *)DUMP_PETZE);
	dope_bind(app_id, "dumper_b_tmpfs", "commit", dump_callback, (void *)DUMP_TMPFS);
	dope_bind(app_id, "dumper_b_dm", "commit", dump_callback, (void *)DUMP_DM);

	/* dataspace entry event handlers */
	dope_bind(app_id, "dumper_e_dm", "press", dump_callback, (void *)DUMP_DM);
	dope_bind(app_id, "dumper_e_dm", "commit", dump_callback, (void *)DUMP_DM);
	dope_bind(app_id, "dumper_e_loader", "press", dump_callback, (void *)DUMP_LOADER);
	dope_bind(app_id, "dumper_e_loader", "commit", dump_callback, (void *)DUMP_LOADER);

	return 0;
}
