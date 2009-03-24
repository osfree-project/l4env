/**
 * \file	loader/server/src/integrity.c
 * \brief	Measure integrity and report it to Lyon virtual TPM.
 *
 * \date	21/07/2007
 * \author	Carsten Weinhold <weinhold@os.inf.tu-dresden.de> */

/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <string.h>
#include <stdlib.h>

#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/util/base64.h>

#include "integrity.h"

#define BASE64_SHA1_DIGEST_SIZE 28

#ifdef USE_INTEGRITY_VTPM
integrity_id_t integrity_nil_id  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#include <l4/stpm/encap.h>
#endif

int
integrity_parse_id(const char *id64, integrity_id_t *id)
{
  l4_size_t size = strlen(id64);
  char     *tmp_buf;

  /* sanity check: we know how long a valid ID has to be */
  if (size != BASE64_SHA1_DIGEST_SIZE)
    return -L4_EINVAL;

  /* XXX hmmm ... base64 functions don't report error codes ... */
  base64_decode(id64, size, &tmp_buf);
  memcpy(id, tmp_buf, sizeof(integrity_id_t));

  free(tmp_buf);

  return 0;
}

void
integrity_hash_data(app_t *app, const char *name, const char *data, size_t size)
{
  crypto_sha1_ctx_t ctx;

  sha1_digest_setup(&ctx);
  sha1_digest_update(&ctx, app->integrity_hash, sizeof(app->integrity_hash));
  if (name)
    sha1_digest_update(&ctx, name, strlen(name));
  sha1_digest_update(&ctx, data, size);
  sha1_digest_final(&ctx, app->integrity_hash);

  app_msg(app, "Hashed %s", name);
}

int
integrity_report_hash(const cfg_task_t *cfg, app_t *app)
{
  int ret;

  if (memcmp(&cfg->integrity.id, integrity_nil_id, sizeof(integrity_id_t)) == 0)
    app_msg(app, "Warning: NIL or no integrity ID given.");

#ifdef USE_INTEGRITY_LYON
  ret = lyon_add(app->owner, cfg->integrity.parent_id,
                 app->tid, cfg->integrity.id,
                 cfg->task.fname, app->integrity_hash);
#else

  ret = stpm_check_server(cfg->integrity.integrity_service, 1);
  if (ret)
    {
      app_msg(app, "Error: vTPM with name '%s' wasn't found.",
              cfg->integrity.integrity_service);
      return -1;
    }

  if (stpm_shutdown_on_exit(&app->tid))
    {
      app_msg(app, "Error: Couldn't register task for exit handling of "
                   "vTPM with name '%s'", cfg->integrity.integrity_service);
      return -1;
    }

  ret = TPM_Extend(14, app->integrity_hash);
#endif
  if (ret < 0)
    {
      app_msg(app, "Warning: Failed to report integrity measurements: %d",
              ret);
    }

  return ret;
}
