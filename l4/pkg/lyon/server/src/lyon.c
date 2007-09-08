/*
 * \brief   Implementation of the lyon interface
 * \date    2006-07-18
 * \author  Bernhard Kauer <kauer@tudos.de>
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * \author  Christelle Braun <cbraun@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007      Christelle Braun <cbraun@os.inf.tu-dresden.de>
 * Copyright (C) 2006-2007 Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * Copyright (C) 2004-2006 Bernhard Kauer <kauer@tudos.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Lyon package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/* generic includes */
#include <malloc.h>
#include <string.h>
#include <assert.h>

/* L4-specific includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/crypto/aes.h>
#include <l4/crypto/cbc.h>
#include <l4/crypto/sha1.h>

#include <l4/stpm/rsaglue.h>

/* local includes */
#include "lyon.h"

/*
 * ***************************************************************************
 */

//#define NDEBUG

#ifdef NDEBUG
# define debug(...) 
#else
# include <stdio.h>
# define debug(...) printf(__VA_ARGS__)
#endif

/*
 * ***************************************************************************
 */

static lyon_pentry_t head = NULL;

char      lyon_aes_key[AES128_KEY_SIZE];
rsa_key_t lyon_rsa_key;
char      lyon_counter_auth[RSA_HASHSIZE];

const lyon_id_t lyon_nil_id = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const char null_iv[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/*
 * ***************************************************************************
 */

#define FIT_SIZE(x) CRYPTO_FIT_SIZE_TO_CIPHER(x, AES_BLOCK_SIZE)
#define DATA_ARRAY_OFFSET(s) (((char *) &s->data[0]) - ((char *) s))

#define FIND_ENTRY(entry, id)		\
{					\
    entry=find_entry(id);		\
    if (entry==NULL)			\
        return -L4_ENOTASK;		\
    assert(entry);			\
}

#define LYON_ID_EQUAL(id0, id1)         \
    (memcmp(id0, id1, sizeof(lyon_id_t)) == 0)

#define COPY_LYON_ID(to, from)          \
    memcpy(to, from, sizeof(lyon_id_t))

/*
 * ***************************************************************************
 */

static inline lyon_pentry_t
find_entry_by_id(const lyon_id_t id)
{
    lyon_pentry_t e = head;

    while (e && !LYON_ID_EQUAL(e->data.id, id))
        e = e->next;
    return e;
}

static inline lyon_pentry_t
find_entry_by_task(l4_threadid_t task)
{
    lyon_pentry_t e = head;

    while (e && !l4_task_equal(e->task, task))
        e = e->next;
    return e;
}

/**
 * Return the number of items in the database.
 *
 * This is only a debugging function.
 */
int
lyon_count(void)
{
    lyon_pentry_t e;
    int count;

    count = 0;
    e = head;
    while (e != NULL)
    {
        count += 1;
        //debug("owner #%x id #%x parent #%x ",
        //      e->owner.id.task, e->data.id.id.task, e->data.parent.id.task);
        //debug("name '%s' hash '%s'\n", e->data.name, e->data.hash);
        e = e->next;
    }
    return count;
}

static void
calc_check_sum(lyon_sealed_data_t *s, unsigned int data_len,
               lyon_hash_t hash_out)
{
    crypto_sha1_ctx_t sha1_ctx;

    sha1_digest_setup(&sha1_ctx);
    sha1_digest_update(&sha1_ctx, (const char *) &s->info, sizeof(s->info));
    sha1_digest_update(&sha1_ctx, (const char *) s->data, data_len);
    sha1_digest_final(&sha1_ctx, hash_out);
}

/*
 * ***************************************************************************
 */

int
lyon_add(l4_threadid_t owner,
         const lyon_id_t parent_id,
         l4_threadid_t new_child,
         const lyon_id_t new_id,
         const char *name,
         const char *hash)
{
    lyon_pentry_t e, p;

    /* if the caller does not create an entirely new branch originating
     * from the root of the tree, we need to do additional checks */
    if ( !LYON_ID_EQUAL(parent_id, lyon_nil_id))
    {
        /* does the caller own the parent node? */
        p = find_entry_by_id(parent_id);
        if (p && !l4_task_equal(p->owner, owner))
            return -L4_EPERM;

        /* does the caller want to add a child of it's own? */
        p = find_entry_by_task(owner);
        if (p && !l4_task_equal(p->task, owner))
            return -L4_EPERM;

        /* does the parent node exists at all? */
        if (p == NULL)
            return -L4_ENOTASK;
    }

    if (find_entry_by_id(new_id) != NULL)
        return -L4_EEXISTS;
    
    e = (lyon_pentry_t) malloc(sizeof(*e));
    if (e == NULL)
        return -L4_ENOMEM;

    e->owner = owner;
    e->task  = new_child;
    COPY_LYON_ID(e->data.id, new_id);
    COPY_LYON_ID(e->data.parent_id, parent_id);
    strncpy(e->data.name, name, sizeof(e->data.name));
    memcpy( e->data.hash, hash, sizeof(e->data.hash));

    e->next = head;
    head    = e;

    assert(find_entry_by_id(new_id) && find_entry_by_task(new_child));

    return 0;
}


int 
lyon_del(l4_threadid_t owner,
         const lyon_id_t id)
{
    lyon_pentry_t e;
    lyon_pentry_t *pe = &head;

    /* find the pointer to the entry that is to be deleted */
    while (*pe && !LYON_ID_EQUAL((*pe)->data.id, id))
        pe = &(*pe)->next;

    if (*pe == NULL)
        return -L4_ENOTASK;

    if (!l4_task_equal((*pe)->owner, owner))
        return -L4_ENOTOWNER;

    /* dequeue and delete the entry */
    e   =  *pe;
    *pe = (*pe)->next;
    free(e);

    /* orphan all childs of the deleted entry by resetting their
     * parent_id values */
    e = head;
    while (e)
    {
        if (LYON_ID_EQUAL(e->data.parent_id, id))
            COPY_LYON_ID(e->data.parent_id, lyon_nil_id);
        e = e->next;
    }

    return 0;
}


int
lyon_link(l4_threadid_t client,
          unsigned int keylen,
          const char *key,
          unsigned int dstlen,
          char *dst)
{
#if 0    
    return lyon_quote(client, keylen, key, dstlen, dst);
#else
    return 0;
#endif
}


int
lyon_quote(l4_threadid_t client,
           unsigned int nlen,
           const char *nonce,
           lyon_quote_t *quote)
{	
    lyon_pentry_t e;
    int res;

    e = find_entry_by_task(client);
    if(e == NULL)
        return -L4_ENOTASK;

    if (quote == NULL)
        return -L4_EINVAL;

    memcpy(&quote->data, &e->data, sizeof(quote->data));
#if 0
    res = rsa_sign(&lyon_rsa_key, sizeof(quote->sig), quote->sig, nlen, nonce,
                   sizeof(quote->data), (char *)&(quote->data), 0);
    if (res >= 0)
        res += sizeof(quote->data);
#else
    memset(quote->sig, 0, sizeof(quote->sig));
    res = 0;
#endif
    return res;
}

int
lyon_extend(l4_threadid_t client,
           unsigned int datalen,
           const char *data)
{	
    lyon_pentry_t e;

    e = find_entry_by_task(client);
    if(e == NULL)
        return -L4_ENOTASK;

    if (data == NULL)
        return -L4_EINVAL;

    crypto_sha1_ctx_t sha1_ctx;

    sha1_digest_setup(&sha1_ctx);
    sha1_digest_update(&sha1_ctx, (const char *) e->data.hash, sizeof(e->data.hash));
    sha1_digest_update(&sha1_ctx, (const char *) data, datalen);
    sha1_digest_final(&sha1_ctx, e->data.hash);

    return 0;
}

int
lyon_seal(l4_threadid_t client,
          const lyon_id_t id,
          unsigned int nlen,
          const char *nonce,
          unsigned int srclen,
          const char *src, 
          unsigned int dstlen,
          char *dst)
{
    lyon_pentry_t       owner, creator;
    lyon_sealed_data_t *s;
    crypto_aes_ctx_t    aes_ctx;
    unsigned int        aes_flags = 0, size;

    size = FIT_SIZE(DATA_ARRAY_OFFSET(s) + nlen + srclen);

    if (src == NULL || dst == NULL || (nonce == NULL && nlen > 0) || size > dstlen)
        return -L4_EINVAL;

    owner   = find_entry_by_id(id);
    creator = find_entry_by_task(client);

    if (owner == NULL || creator == NULL)
        return -L4_ENOTASK;

    s = (lyon_sealed_data_t *) malloc(size);
    if (s == NULL)
        return -L4_ENOMEM;

    /* we need to zero out at least the padding bytes at the end of the
     * buffer */
    memset(s, 0, size);

    /* FIXME: monotonic counter support is missing */
    /* FIXME: We depend on the client to provide good nonces. What about
     *        adding our own nonce, maybe even hashing all nonces into a
     *        single hash sum? */

    /* marshal all user-provided data and the info inserted by Lyon */
    COPY_LYON_ID(s->info.creator, creator->data.id);
    COPY_LYON_ID(s->info.owner, owner->data.id);
    s->info.version     = LYON_INVALID_VERSION;
    s->info.data_offset = nlen;
    s->info.data_len    = srclen;
    if (nonce)
        memcpy(&s->data[0], nonce, nlen);
    memcpy(&s->data[nlen], src, srclen);

    /* calculate sanity checksum */
    calc_check_sum(s, nlen + srclen, s->checksum);

    /* encrypt the all data and info */
    aes_cipher_set_key(&aes_ctx, lyon_aes_key, AES128_KEY_SIZE, &aes_flags);
    crypto_cbc_encrypt(aes_cipher_encrypt, &aes_ctx, AES_BLOCK_SIZE,
                       (const char *) s, dst, null_iv, size);
    free(s);

    return size;
}


int
lyon_unseal(l4_threadid_t client,
            unsigned int srclen,
            const char *src,
            unsigned int dstlen,
            char *dst)
{
    lyon_pentry_t       e;
    lyon_sealed_data_t *s = (lyon_sealed_data_t *) dst;
    crypto_aes_ctx_t    aes_ctx;
    lyon_hash_t         real_checksum;
    unsigned int        aes_flags = 0, data_len;

    if (src == NULL || dst == NULL || srclen < sizeof(*s) ||
        srclen > dstlen || srclen % AES_BLOCK_SIZE != 0)
        return -L4_EINVAL;

    e = find_entry_by_task(client);
    if (e == NULL)
        return -L4_ENOTASK;

    /* decrypt data into output buffer */
    aes_cipher_set_key(&aes_ctx, lyon_aes_key, AES128_KEY_SIZE, &aes_flags);
    crypto_cbc_decrypt(aes_cipher_decrypt, &aes_ctx, AES_BLOCK_SIZE,
                       src, (char *) s, null_iv, srclen);

    /* extract the total length of the data that has to be checksummed */
    data_len = s->info.data_offset + s->info.data_len;
    if (data_len > srclen - sizeof(*s))
    {
        memset(dst, 0, srclen);
        return -L4_EINVAL;
    }

    /* calculate and verify sanity checksum */
    calc_check_sum(s, data_len, real_checksum);
    if (memcmp(real_checksum, s->checksum, sizeof(real_checksum)) != 0)
    {
        memset(dst, 0, srclen);
        return -L4_EINVAL;
    }
    
    /* finally, check owner permission */
    if ( !LYON_ID_EQUAL(e->data.id, s->info.owner))
    {
        memset(dst, 0, srclen);
        return -L4_EPERM;
    }

    return sizeof(*s) + s->info.data_offset + s->info.data_len;
}

