/*
 * \brief   BMSI integrity interface implementation.
 * \date    2007-07-14
 * \author  Christelle Braun <cbraun@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007 Christelle Braun <cbraun@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the BMSI package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>

#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/lyon/lyon.h>

#include <l4/bmsi/bmsi.h>
#include <l4/bmsi/bmsi-oo.h>
#include "bmsi-client.h"


extern 
int *IIFaces_Map[MAX_IIFACES];

int 
IntegrityInterface_extend(IntegrityInterface Self, 
                          int Hash_size, 
                          byte *Hash)
{
    /* Ask Lyon for the extend operation. */
    int res_extend = lyon_extend(l4_myself(),
                          Hash_size,
                          (const char*)Hash);


    return res_extend;
} 

int 
IntegrityInterface_seal(IntegrityInterface Self, 
                        int Secret_size, 
                        byte *Secret, 
                        int *Blob_size, 
                        byte **Blob)
{
    /* Ask Lyon for a quote. */
    lyon_quote_t quote;
    char *nonce = "nonce";
    int res_quote;  
    res_quote = lyon_quote(L4_INVALID_ID, strlen(nonce), nonce, &quote);

    /* Find the lyon_id. */
    lyon_id_t my_lyon_id;
    memcpy(my_lyon_id, quote.data.id, sizeof(lyon_id_t));

    /* Seal the secret data. */
    const lyon_hash_t seal_hash = "seal_test_hash01234\0";
    int res_seal;

    res_seal = lyon_seal(L4_INVALID_ID, 
                         my_lyon_id, 
                         sizeof(seal_hash), 
                         seal_hash,
                         Secret_size, 
                         (const char *)Secret, 
                         *Blob_size, 
                         (char *)Blob);

    if(res_seal<0)
        return res_seal;

    *Blob_size = res_seal;

    return 0;
}

int 
IntegrityInterface_unseal(IntegrityInterface Self, 
                          int Blob_size, 
                          byte *Blob,
                          int *Data_size,
                          byte **Data)
{
    /* Allocate the buffer where the secret will be unsealed. */
    *Data = malloc(1024);

    /* Unseal the secret. */
    int res_unseal;

    res_unseal = lyon_unseal(L4_INVALID_ID,
                             Blob_size,
                             (const char *)Blob,
                             1024,
                             (char *)*Data);

    if(res_unseal<0)
        return res_unseal;

    *Data_size = res_unseal;
    
    return 0;
} 


int 
IntegrityInterface_quote(IntegrityInterface Self, 
                         int ExternalData_size,
                         byte *ExternalData,
                         int *Proof_size,
                         byte **Proof)
{

    l4_threadid_t me = l4_myself();
    lyon_quote_t *quote = (lyon_quote_t*)Proof;

    *Proof_size = sizeof(lyon_quote_t);

    int res_quote = lyon_quote(me,
                               ExternalData_size,
                               (char *)ExternalData,
                               quote);

    return res_quote;
}

int
IntegrityInterface_free(IntegrityInterface Self)
{
    /* Remove the entry in the list.*/
    int i=0;
    for(i=0; i<MAX_IIFACES; i++)
    {
        if(IIFaces_Map[i]!=NULL && IIFaces_Map[i]==Self)
        {
            free(IIFaces_Map[i]);
            return 0;
        }
    }

    LOG("ERROR: The IIFace at (%p) was not found!", (int*)Self);

    return -1;
}

