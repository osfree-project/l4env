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
  memcpy(*id, tmp_buf, sizeof(lyon_id_t));

  free(tmp_buf);

  return 0;
}

void
integrity_hash_data(app_t *app, const char *name, const unsigned char *data, size_t size)
{
  crypto_sha1_ctx_t ctx;

  sha1_digest_setup(&ctx);
  sha1_digest_update(&ctx, (const unsigned char *)app->integrity_hash, sizeof(app->integrity_hash));
  if (name)
    sha1_digest_update(&ctx, (const unsigned char *)name, strlen(name));
  sha1_digest_update(&ctx, data, size);
  sha1_digest_final(&ctx, (unsigned char *)app->integrity_hash);

  app_msg(app, "Hashed %s", name);
}

int
integrity_report_hash(const cfg_task_t *cfg, app_t *app)
{
  int ret;

  if (memcmp(cfg->integrity.id, lyon_nil_id, sizeof(lyon_nil_id)) == 0)
    app_msg(app, "Warning: NIL or no integrity ID given.");

  ret = lyon_add(app->owner, cfg->integrity.parent_id,
                 app->tid, cfg->integrity.id,
                 cfg->task.fname, app->integrity_hash);
  if (ret < 0)
    {
      app_msg(app, "Warning: Failed to report integrity measurements: %d",
              ret);
    }

  return ret;
}
