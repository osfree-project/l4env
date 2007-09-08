/*
 * \brief   Header for Lyon client interface.
 * \date    2006-07-17
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * \author  Christelle Braun <cbraun@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006-2007  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _LYON_LYONIF_H
#define _LYON_LYONIF_H

#include <l4/sys/types.h>
#include <l4/stpm/rsaglue.h>

/*
 * ***************************************************************************
 */

#define LYON_ROOT_NAME "LYON_ROOT"
#define LYONIF_NAME    "lyon"

#define LYON_INVALID_VERSION 0

/*
 * ***************************************************************************
 */

typedef char lyon_hash_t[20];
typedef lyon_hash_t   lyon_id_t;

typedef struct lyon_data_s
{
    lyon_id_t   id;
    lyon_id_t   parent_id;
    char        name[40];
    lyon_hash_t hash;
} lyon_data_t;

typedef struct lyon_entry_s *lyon_pentry_t;
struct lyon_entry_s
{
    lyon_data_t          data;
    l4_threadid_t        owner;
    l4_threadid_t        task;
    struct lyon_entry_s *next;
};

typedef struct lyon_quote_s
{
    lyon_data_t data;
    char        sig[MAX_RSA_MODULUS_LEN];
} lyon_quote_t;

typedef struct lyon_sealed_data_s
{
    lyon_hash_t checksum;
    struct
    {
        lyon_id_t     creator;
        lyon_id_t     owner;
        unsigned long version;
        unsigned int  data_offset;
        unsigned int  data_len;
    } info;
    unsigned char data[0];

} lyon_sealed_data_t;

/*
 * ***************************************************************************
 */

extern char      lyon_aes_key[];
extern rsa_key_t lyon_rsa_key;
extern char      lyon_counter_auth[];

extern const lyon_id_t lyon_nil_id;

/*
 * ***************************************************************************
 */

/** \defgroup lyon Minimal Virtual TPM Client Interface */

/**
 * @brief Add a new node into the measurements database.
 *
 * @ingroup lyon
 *
 * This function adds a new child node of a given parent node into the
 * measurements database. The parent node must be either the root node, any
 * node previously created by the caller, or the node representing the caller
 * himself.
 *
 * @param owner     The task that owns the new node (i.e., the caller).
 * @param parent_id The Lyon ID specifying the parent in the tree. Use
 *                  LYON_NIL_ID when creating the root node.
 * @param new_child The task the the new node corresponds to.
 * @param new_id    The Lyon ID that names the new node.
 * @param name      An arbitrary name that will also be included in a quote.
 * @param hash      An arbitrary hash that will also be included in a quote.
 *                  This is usually the hash of the binary code and possibly
 *                  the configuration data for the corresponding task.
 *
 * @return The error code of the operation:
 *         0            Success.
 *         -L4_ENOPERM  The parent node is not owned by the caller or it does
 *                      not describe the caller himself.
 *         -L4_ENOTASK  No such parent.
 *         -L4_EEXISTS  There already is a node with this Lyon ID.
 *         -L4_ENOMEM   Out of memory.
 */
int
lyon_add   (l4_threadid_t owner,
	    const lyon_id_t parent_id,
            l4_threadid_t new_child,
	    const lyon_id_t new_id,
	    const char *name,
	    const char *hash);

/**
 * @brief Delete a node from the measurements database.
 *
 * @ingroup lyon
 *
 * This function removes a node from the database. The parent-value of all
 * children is set LYON_NIL_ID meaning they all become orphants.
 *
 * @param owner  The task that owns the node.
 * @param id     The Lyon ID specifying the node that should be removed.
 *
 * @return The error code of the operation:
 *         - 0           Success.
 *         - L4_ENOTASK  There is no node with the given Lyon ID.
 */
int 
lyon_del   (l4_threadid_t owner,
	    const lyon_id_t id);

int
lyon_link  (l4_threadid_t client,
	    unsigned int keylen,
	    const char *key,
	    unsigned int dstlen,
	    char *dst);

/**
 * @brief Request a quote of a nonce for a given client.
 *
 * @ingroup lyon
 *
 * This function signs a nonce for the given task using Lyon's signature
 * key.
 *
 * @param client The task, for which the nonce will be signed.
 * @param nlen   The length of the nonce data.
 * @param nonce  The nonce data, the hash of which will be signed.
 * @param quote  The resulting quote that is delivered back to the caller.
 *
 * @return The error code of the operation:
 *         0            Success.
 *         -L4_ENOTASK  There is no such task.
 */
int
lyon_quote (l4_threadid_t client,
	    unsigned int nlen,
	    const char *nonce,
	    lyon_quote_t *quote);

/**
 * @brief Update the integrity measurement.
 *
 * @ingroup lyon
 *
 * This function updates the integrity measurement based on both the 
 * current value of the registered hash and the data given as parameter.
 *
 * @param client      The task that requests the extend operation.
 * @param datalen     The length of the added data.
 * @param data        The added data.
 *
 * @return The error code of the operation:
 *         0            Success.
 *         -L4_ENOTASK  There is no such task.
 */
int
lyon_extend (l4_threadid_t client,
	    unsigned int datalen,
	    const char *data);

/**
 * @brief Seal data and a nonce for the given client.
 *
 * @ingroup lyon
 *
 * This function encrypts data (prepended by the nonce) using Lyon's
 * encryption key. The data will be sealed for the specified client, meaning
 * Lyon will not make the data available to any task that has a different
 * Lyon ID than the one specified for the seal operation. The identity of the
 * sealer (i.e., its Lyon ID) will be embedded into the sealed blob, so that
 * it can be checked after unsealing.
 *
 * @param client The task that requests the seal operation.
 * @param id     The Lyon ID that names the task that should be allowed to
 *               unseal the data.
 * @param nlen   The length of the nonce data.
 * @param nonce  The nonce data that will be used as salt for the encryption.
 * @param srclen The length of the data to be sealed.
 * @param src    The data to be sealed.
 * @param dstlen The size of the output buffer for the sealed data.
 * @param dst    The sealed data blob.
 *
 * @return If the operation was successful, the number of bytes written to the
 *         output buffer is returned, otherwise the error code of the
 *         operation:
 *         -L4_ENOTASK  Either client or id could not be mapped to a node
 *                      in the measurements database.
 *         -L4_EINVAL   The paremters concerning nonce, input data, and output
 *                      buffer are invalid.
 *         -L4_ENOMEM   Failed to allocate a temporary buffer.
 */
int
lyon_seal  (l4_threadid_t client,
            const lyon_id_t id,
            unsigned int nlen,
            const char *nonce,
            unsigned int srclen, 
            const char *src,
            unsigned int dstlen,
            char *dst);

/**
 * @brief Unseal data for the given client.
 *
 * @ingroup lyon
 *
 * This function decrypts data that has been sealed for the given task. Also
 * see lyon_seal().
 *
 * @param client The task that requests the unseal operation.
 * @param srclen The length of the data blob to be unsealed.
 * @param src    The data blob to be sealed.
 * @param dstlen The size of the output buffer for the unsealed data.
 * @param dst    The unsealed data.
 *
 * @return If the operation was successful, the number of bytes written to the
 *         output buffer is returned, otherwise the error code of the
 *         operation:
 *         -0           Success.
 *         -L4_ENOTASK  The client is registered in the measurements database
 *                      and is therefore not allowed to get any data unsealed.
 *         -L4_EINVAL   The paremters concerning input data or the output
 *                      buffer are invalid.
 *         -L4_ENOPERM  The data has not been sealed for the given client.
 */
int
lyon_unseal(l4_threadid_t client,
            unsigned int srclen,
	    const char *src, 
	    unsigned int dstlen,
	    char *dst);

#endif
