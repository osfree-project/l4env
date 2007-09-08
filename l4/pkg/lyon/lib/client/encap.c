/*
 * \brief   Macro-encoded client IDL stubs for Lyon interface.
 * \date    2006-07-17
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Lyon package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/lyon/lyon-client.h>
#include <l4/lyon/lyon.h>
#include <l4/stpm/encap.h>

const lyon_id_t lyon_nil_id = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


CHECK_SERVER(LYONIF_NAME);

ENCAP_FUNCTION(lyon,
               add,
               (l4_threadid_t owner,
                const lyon_id_t parent_id,
                l4_threadid_t new_child,
                const lyon_id_t new_id,
                const char *name,
                const char *hash),
               parent_id,
               &new_child,
               new_id,
               name,
               hash);
                     

ENCAP_FUNCTION(lyon,
               del,
               (l4_threadid_t owner,
                const lyon_id_t new_id),
               new_id);

ENCAP_FUNCTION(lyon,
               link,
               (l4_threadid_t client,
                unsigned int keylen,
                const char *key,
                unsigned int dstlen,
                char *dst),
               keylen,
               key,
               &dstlen,
               &dst);

ENCAP_FUNCTION(lyon,
               quote,
               (l4_threadid_t client,
                unsigned int nlen,
                const char *nonce,
                lyon_quote_t *quote),
               nlen,
               nonce,
               quote);

ENCAP_FUNCTION(lyon,
               extend,
               (l4_threadid_t client,
                unsigned int datalen,
                const char *data),
               datalen,
               data);

ENCAP_FUNCTION(lyon,
               seal,
               (l4_threadid_t client,
                const lyon_id_t id,
                unsigned int hashlen,
                const char *hash,
                unsigned int srclen,
                const char *src,
                unsigned int dstlen,
                char *dst),
               id,
               hashlen,
               hash,
               srclen,
               src,
               &dstlen,
               &dst);

ENCAP_FUNCTION(lyon,
               unseal,
               (l4_threadid_t client,
                unsigned int srclen,
                const char *src,
                unsigned int dstlen,
                char *dst),
               srclen,
               src,
               &dstlen,
               &dst);
