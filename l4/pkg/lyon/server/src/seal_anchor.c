/*
 * \brief   Seal and unseal the anchor secrets using the TPM.
 * \date    2006-07-14
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

/* generic includes */
#include <stdlib.h>
#include <string.h>

/* L4-specific includes */
#include <l4/log/l4log.h>
#include <l4/crtx/ctor.h>
#include <l4/crypto/aes.h>
#include <l4/crypto/sha1.h>
#include <l4/crypto/cbc.h>

#include <l4/stpm/cryptoglue.h>
#include <l4/stpm/rsaglue.h>
#include <l4/stpm/tcg/tpm.h>
#include <l4/stpm/tcg/seal.h>
#include <l4/stpm/tcg/ord.h>

#if USE_TFS_STUB
# include <l4/trustedfs/trustedfs_block.h>
#endif

/*
 * ***************************************************************************
 */

#define SRK_HANDLE      0x40000000
#define SEAL_PCR_MAP    0x00000000

#define MAX_BLOB_SIZE   512
#define ENC_STAGE2_SIZE \
        CRYPTO_FIT_SIZE_TO_CIPHER(sizeof(secrets_stage2_t), AES_BLOCK_SIZE)

/* US_* means UNTRUSTED_STORAGE_* ... */
#define US_FS_NAME  "blac_fs"
#define US_FILENAME "sealed-memory-root.dat"

/*
 * ***************************************************************************
 */

/* The sealed secrets are split into two parts: stage 1 is relatively small so
 * that it can be sealed/unsealed with just one call to
 * TPM_Seal()/TPM_Unseal(). It contains the AES key needed to decrypt stage 2
 * and a check sum of itself and the encrypted stage 2.
 *
 * Stage 2 contains the auth value of the monotonic counter and the RSA key,
 * which is too large to be sealed/unsealed using the TPM within an acceptable
 * time frame.
 */

typedef struct secrets_stage1_s
{
    char         check_sum[TCG_HASH_SIZE];
    char         aes_key[AES128_KEY_SIZE];
} secrets_stage1_t;


typedef struct secrets_stage2_s
{
    rsa_key_t rsa_key;
    char      counter_auth[TCG_HASH_SIZE];
} secrets_stage2_t;


typedef struct sealed_secrets_s
{
    unsigned int  stage1_blob_size;
    char stage1_blob[MAX_BLOB_SIZE];
    char stage2_blob[ENC_STAGE2_SIZE];
} sealed_secrets_t;

/*
 * ***************************************************************************
 */

static const char null_iv[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

#if USE_TFS_STUB
static tfs_bs_t *bs;
#else
/* this is a hack: we keep the result of the last sealing operation in memory */
static sealed_secrets_t sealed_memory;
#endif

/*
 * ***************************************************************************
 */

static void
calc_check_sum(const secrets_stage1_t *stage1, const char *enc_stage2, char *hash_out)
{
    crypto_sha1_ctx_t ctx;

    sha1_digest_setup( &ctx);
    sha1_digest_update(&ctx, stage1->aes_key, sizeof(stage1->aes_key));
    sha1_digest_update(&ctx, enc_stage2, ENC_STAGE2_SIZE);
    sha1_digest_final( &ctx, hash_out);
}


int
seal_secrets(const char *srk_auth, const char *secrets_auth,
             const char *aes_key, rsa_key_t *rsa_key, char *counter_auth)
{
    char              s2_buf[ENC_STAGE2_SIZE];
    secrets_stage1_t  s1;
    secrets_stage2_t *s2;
    sealed_secrets_t *ss;
    crypto_aes_ctx_t  aes_ctx;
    unsigned int      aes_flags = 0;
    int               ret;
    
    /* we put the secrets_stage2_t structure into a buffer whose size is a
     * multiple of the cipher block size, because it is to be used a input
     * for the CBC encrypt function */
    
    s2 = (secrets_stage2_t *) s2_buf;
#if USE_TFS_STUB
    ss = (sealed_secrets_t *) bs->block_buf;
#else
    ss = &sealed_memory;
#endif
    
    /* wrap secrets in stages 1 and 2 and encrypt the second part using
     * the AES key */
    memcpy(&s1.aes_key, aes_key, sizeof(s1.aes_key));
    memcpy(&s2->rsa_key, rsa_key, sizeof(s2->rsa_key));
    memcpy(s2->counter_auth, counter_auth, sizeof(s2->counter_auth));

    aes_cipher_set_key(&aes_ctx, aes_key, AES128_KEY_SIZE, &aes_flags);
    crypto_cbc_encrypt(aes_cipher_encrypt, &aes_ctx, AES_BLOCK_SIZE,
                       s2_buf, ss->stage2_blob, null_iv, sizeof(s2_buf));

    /* calculate plausibilty check sum */
    calc_check_sum(&s1, ss->stage2_blob, s1.check_sum);

    /* now use the TPM to seal the secrets completely */
    ret = TPM_Seal_CurrPCR(SRK_HANDLE, SEAL_PCR_MAP,
                           (unsigned char *) srk_auth, (unsigned char *) secrets_auth,
                           (unsigned char *) &s1, sizeof(s1),
                           (unsigned char *) ss->stage1_blob, &ss->stage1_blob_size);
    
    LOG("TPM_Seal_CurrPCR(): ret=%d", ret);
    if (ret != 0)
        return ret;

    /* finally, write the sealed secrets to untrusted storage */
#if USE_TFS_STUB
    return _tfs_bs_rw_sealed(bs, US_FILENAME, sizeof(*ss), TFS_BS_WRITE_MODE);
#else
    return 0;
#endif
}


int
unseal_secrets(char *srk_auth, char *secrets_auth,
               char *aes_key_out, rsa_key_t *rsa_key_out,
               char *counter_auth_out)
{
    unsigned int      stage1_size;
    char              real_check_sum[TCG_HASH_SIZE];
    char              s1_buf[MAX_BLOB_SIZE];
    char              s2_buf[ENC_STAGE2_SIZE];
    secrets_stage1_t *s1;
    secrets_stage2_t *s2;
    sealed_secrets_t *ss;
    crypto_aes_ctx_t  aes_ctx;
    unsigned int      aes_flags = 0;
    int               ret;
    
    /* we put the secrets_stage2_t structure into a buffer whose size is a
     * multiple of the cipher block size, because it is to be used a input
     * for the CBC encrypt function */
    
    s1 = (secrets_stage1_t *) s1_buf;
    s2 = (secrets_stage2_t *) s2_buf;
#if USE_TFS_STUB
    ss = (sealed_secrets_t *) bs->block_buf;
#else
    ss = &sealed_memory;
#endif
    
    /* read sealed secrets from untrusted storage */
#if USE_TFS_STUB
    ret = _tfs_bs_rw_sealed(bs, US_FILENAME, sizeof(*ss), TFS_BS_READ_MODE);
    if (ret != 0)
        return ret;
#endif

    /* unseal secrets */
    if (ss->stage1_blob_size > MAX_BLOB_SIZE)
        return -1; /* illegal data length */

    ret = TPM_Unseal(SRK_HANDLE, (unsigned char *) srk_auth, (unsigned char *) secrets_auth,
                     (unsigned char *) ss->stage1_blob, ss->stage1_blob_size,
                     (unsigned char *) s1_buf, &stage1_size);
    LOG("TPM_Unseal(): ret=%d", ret);
    if (ret != 0)
        return ret;
    
    if (stage1_size != sizeof(*s1))
        return -1; /* illegal length of unsealed data */

    /* plausibility check: do the secrets match check sum? */
    calc_check_sum(s1, ss->stage2_blob, real_check_sum);
    if (memcmp(real_check_sum, s1->check_sum, TCG_HASH_SIZE) != 0)
        return -1; /* inconsistent data */

    aes_cipher_set_key(&aes_ctx, s1->aes_key, AES128_KEY_SIZE, &aes_flags);
    crypto_cbc_decrypt(aes_cipher_decrypt, &aes_ctx, AES_BLOCK_SIZE,
                       ss->stage2_blob, s2_buf, null_iv, sizeof(s2_buf));
    
    /* unwrap the secrets */
    memcpy(aes_key_out, s1->aes_key, sizeof(s1->aes_key));
    memcpy(rsa_key_out, &s2->rsa_key, sizeof(s2->rsa_key));
    memcpy(counter_auth_out, s2->counter_auth, sizeof(s2->counter_auth));

    return 0;
}

/*
 * ***************************************************************************
 */

static void
seal_anchor_init(void)
{
#if USE_TFS_STUB
    bs = _tfs_bs_init(US_FS_NAME);
    if (bs == NULL)
        exit(1);
#endif
}

L4C_CTOR(seal_anchor_init, L4CTOR_AFTER_BACKEND);
