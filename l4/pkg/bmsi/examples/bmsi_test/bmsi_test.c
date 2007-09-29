/*
 * \brief   BMSI test case.
 * \date    2007-07-14
 * \author  Christelle Braun <cbraun@os.inf.tu-dresden.de>
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007 Christelle Braun <cbraun@os.inf.tu-dresden.de>
 * Copyright (C) 2007 Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the BMSI package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <l4/log/l4log.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/names/libnames.h>
#include <l4/dm_generic/dm_generic.h>
#include <l4/env/errno.h>
#include <l4/util/util.h>
#include <l4/lyon/lyon.h>

#include <l4/bmsi/bmsi-oo.h>
#include <l4/bmsi/bmsi.h>

int
DBG_pdommap(void);

int
DBG_iifacemap(int n);

static void
print_result(const char *text, int no)
{
    printf("%s %s: %d (%s)\n", (no != 0) ? "FAIL:" : "OK:  ", text, no, l4env_strerror(no));
}

static void
print_bytes(const char *prefix, const char *format,
            const char *data, unsigned int len)
{
    int i;
    
    printf("%s", prefix);
    for (i = 0; i < len; i++)
    {
        printf(format, (unsigned char)data[i]);
        if (i % 20 == 19 && i + 1 != len)
            printf("\n%s", prefix);
    }
    printf("\n");
}

static int
test_quote_and_extend(IntegrityInterface iif)
{
    lyon_quote_t quote;
    int          quotelen;
    char        *nonce = "nonce";

    lyon_id_t id1, id2, id3;
    lyon_hash_t hash1, hash2, hash3;
    
    /* Quote initial id and data. */
    int res_quote1 = IntegrityInterface_quote(iif,
                                              strlen(nonce),
                                              (byte *) nonce,
                                              &quotelen,
                                              (byte **) &quote);
    
    memcpy(id1, quote.data.id, sizeof(lyon_id_t));
    memcpy(hash1, quote.data.hash, sizeof(lyon_hash_t));

    LOGd(DBG_TEST, "res_quote1(%d) -- quotelen(%d)", 
         res_quote1,
         quotelen);
    print_bytes("id_1   : ", "%02x ", id1, sizeof(id1));
    print_bytes("hash_1 : ", "%02x ", hash1, sizeof(hash1));
    
    /* Quote again and check id and data. */
    int res_quote2 = IntegrityInterface_quote(iif,
                                              strlen(nonce),
                                              (byte *)nonce,
                                              &quotelen,
                                              (byte **) &quote);

    memcpy(id2, quote.data.id, sizeof(lyon_id_t));
    memcpy(hash2, quote.data.hash, sizeof(lyon_hash_t));
    
    LOGd(DBG_TEST, "res_quote2(%d) -- quotelen(%d)", 
         res_quote2,
         quotelen);
    print_bytes("id_2  : ", "%02x ", id2, sizeof(id2));
    print_bytes("hash_2: ", "%02x ", hash2, sizeof(hash2));
    
    print_result("id_1 == id_2?: ",
                 memcmp(id1, id2, sizeof(lyon_id_t)));
    
    print_result("hash_1 == hash_2?: ",
                 memcmp(hash1, hash2, sizeof(lyon_hash_t)));

    /* Extend data. */
    char *data = "my_data";
    unsigned int datalen = strlen(data);

    int res_extend = IntegrityInterface_extend(iif,
                                               datalen,
                                               (byte *)data);

    LOGd(DBG_TEST, "res_extend(%d)", res_extend);

    /* Quote again and check id and data. */
    int res_quote3 = IntegrityInterface_quote(iif,
                                              strlen(nonce),
                                              (byte *)nonce,
                                              &quotelen,
                                              (byte **) &quote);
    
    memcpy(id3, quote.data.id, sizeof(lyon_id_t));
    memcpy(hash3, quote.data.hash, sizeof(lyon_hash_t));
    
    LOGd(DBG_TEST, "res_quote3(%d) -- quotelen(%d)", 
         res_quote3,
         quotelen);
    print_bytes("id_3  : ", "%02x ", id3, sizeof(id3));
    print_bytes("hash_3: ", "%02x ", hash3, sizeof(hash3));
    
    print_result("id_2 == id_3?: ",
                 memcmp(id2, id3, sizeof(lyon_id_t)));
    
    print_result("hash_2 != hash_3?: ",
                 !memcmp(hash2, hash3, sizeof(lyon_hash_t)));

    return 0;
}

static int
test_libb(void)
{
    /* Get the BMSI versioin. */
    int Version = 0;
    int res_version = BMSI_getVersion(&Version);
    LOGd(DBG_TEST, "res_version(%d) -- Version(%d)", res_version, Version);

    /* Create a PDBuilder instance ('Builder'). */
    BuilderInitializer BuilderParams = "COPYFS:BMODFS";
    PDBuilder Builder;
    int res_builder = BMSI_getBuilder(BuilderParams, &Builder);
    LOGd(DBG_TEST, "res_builder(%d) -- Builder(%d)", res_builder, (L4PDBuilder)Builder);

    /* Create a PD instance ('PD1') with the PDBuilder 'Builder'. */
    DomainInitializer PDParams1 = "";
    ProtectionDomain PD1;
    int res_pd1 = PDBuilder_createPD(Builder, PDParams1, &PD1);
    LOGd(DBG_TEST, "res_pd1(%d) -- PD1(%d)", res_pd1, (L4PD)PD1);

    /* Create a PD instance ('PD2') with the PDBuilder 'Builder'. */
    DomainInitializer PDParams2 = "";
    ProtectionDomain PD2;
    int res_pd2 = PDBuilder_createPD(Builder, PDParams2, &PD2);
    LOGd(DBG_TEST, "res_pd2(%d) -- PD2(%d)", res_pd2, (L4PD)PD2);
    
    /* Print the map of PD instances. */
    if(DBG_TEST)
        DBG_pdommap();

    /* Set Resources for PD1. */
    Resources Res1;
    Res1.MemoryMax = 4096;
    Res1.MemoryCurrent = 2048;
    Res1.Priority = 1;
    Res1.NbrCpu = 1;
    int res_res1 = ProtectionDomain_setResources(PD1, Res1);
    LOGd(DBG_TEST, "res_res1(%d) -- MemMax(%ld)-MemCur(%ld)-Prio(%d)-NbCpus(%d)",
         res_res1, 
         Res1.MemoryMax,
         Res1.MemoryCurrent,
         Res1.Priority,
         Res1.NbrCpu);

    /* Set Resources for PD2. */
    Resources Res2;
    Res2.MemoryMax = 2048;
    Res2.MemoryCurrent = 1024;
    Res2.Priority = 4;
    Res2.NbrCpu = 1;
    int res_res2 = ProtectionDomain_setResources(PD2, Res2);
    LOGd(DBG_TEST, "res_res2(%d) -- MemMax(%ld)-MemCur(%ld)-Prio(%d)-NbCpus(%d)",
         res_res2, 
         Res2.MemoryMax,
         Res2.MemoryCurrent,
         Res2.Priority,
         Res2.NbrCpu);

    /* Get Resources for PD1. */
    Resources Res1get;
    int res_res1get = ProtectionDomain_getResources(PD1, &Res1get);
    LOGd(DBG_TEST, "res_res1get(%d) -- MemMax(%ld)-MemCur(%ld)-Prio(%d)-NbCpus(%d)",
         res_res1get, 
         Res1get.MemoryMax,
         Res1get.MemoryCurrent,
         Res1get.Priority,
         Res1get.NbrCpu);

    /* Print the map of PD instances. */
    if(DBG_TEST) 
        DBG_pdommap();

    /* Build PD1. */
    PDDescriptor Image1 = malloc(sizeof(L4PDDescriptor));
    ((L4PDDescriptor*)Image1)->Name = "hello_test";
    ((L4PDDescriptor*)Image1)->Memory = 16;
    ((L4PDDescriptor*)Image1)->L4Lx = 0;
    ((L4PDDescriptor*)Image1)->Params = "arg1 arg2";
    int res_build1 = ProtectionDomain_build(PD1, Image1);
    LOGd(DBG_TEST, "res_build1(%d)", res_build1);

    /* Get LocalID of PD2. */
    int64 PD2id;
    int res_PD2id = ProtectionDomain_getLocalId(PD2, &PD2id);
    LOGd(DBG_TEST, "res_PD2id(%d) -- PD2id(%lld)", res_PD2id, PD2id); 

    /* Find PD2. */
    ProtectionDomain PD2find;
    int res_PD2find = PDBuilder_findPD(Builder, PD2id, &PD2find); 
    LOGd(DBG_TEST, "res_PD2find(%d) -- PD2find(%p)", res_PD2find, PD2find);

    /* Build PD2. */
    PDDescriptor Image2 = malloc(sizeof(L4PDDescriptor));
    ((L4PDDescriptor*)Image2)->Name = "hello_test2";
    ((L4PDDescriptor*)Image2)->Memory = 16;
    ((L4PDDescriptor*)Image2)->L4Lx = 0;
    ((L4PDDescriptor*)Image2)->Params = "arg1 arg2";
    int res_build2 = ProtectionDomain_build(PD2, Image2);
    LOGd(DBG_TEST, "res_build2(%d)", res_build2);

#if 0
    /* Allow connection PD1 -> PD2. */
    l4_sleep(5000);
    int res_con1 = ProtectionDomain_allowConnection(PD1, PD2, 1);
    LOGd(DBG_TEST, "res_con1(%d)", res_con1);
 
    /* Allow connection PD2 -> PD1. */
    l4_sleep(10000);
    int res_con2 = ProtectionDomain_allowConnection(PD2, PD1, 1);
    LOGd(DBG_TEST, "res_con2(%d)", res_con2);
#endif 

  
    /* Destroy PD1. */
    l4_sleep(10000);
    int res_destroy = ProtectionDomain_destroy(PD1);
    LOGd(DBG_TEST, "res_destroy(%d)", res_destroy);

    /* Get LocalID of PD1. */
    int64 PD1id;
    int res_PD1id = ProtectionDomain_getLocalId(PD1, &PD1id);
    LOGd(DBG_TEST, "res_PD1id(%d) -- PD1id(%lld)", res_PD1id, PD1id); 

    /* Try to find PD1. */
    ProtectionDomain PD1find;
    int res_PD1find = PDBuilder_findPD(Builder, PD1id, &PD1find); 
    LOGd(DBG_TEST, "res_PD1find(%d) -- PD1find(%p)", res_PD1find, PD1find);

    /* Free PD1. */

    /* Destroy PDBuilder. */
    l4_sleep(10000);
    int res_PDBuilder_free = PDBuilder_free(Builder);
    LOGd(DBG_TEST, "res_PDBuilder_free(%d)", res_PDBuilder_free);
 
    return 0;
}

static int 
test_libu(void)
{
 
    if(DBG_TEST)
        DBG_iifacemap(0);
 
    /* Create an IntegrityInterface instance IIFace0. */
    IntegrityInterface IIFace0; 
    int res_iif0 = BMSI_getIntegrityInterface(&IIFace0);
    LOGd(DBG_TEST, "res_iif0(%d) -- IIFace0(%d), ref(%p) saved at(%p)", 
         res_iif0, 
         *((int*)IIFace0), 
         (int*)IIFace0,
         &IIFace0);
    
    if(DBG_TEST)
        DBG_iifacemap(1);

    /* Create an IntegrityInterface instance IIFace1. */
    IntegrityInterface IIFace1;
    int res_iif1 = BMSI_getIntegrityInterface(&IIFace1);
    LOGd(DBG_TEST, "res_iif1(%d) -- IIFace1(%d), ref(%p) saved at(%p)", 
         res_iif1, 
         *((int*)IIFace1),
         (int*)IIFace1,
         &IIFace1);
    
    if(DBG_TEST)
        DBG_iifacemap(2);

    /* Seal secret. */
    char *secret1 = "My data1 to save";
    int secretsize1 = strlen(secret1);
    unsigned char *blob1;
    int blobsize1 = sizeof(blob1);

    int res_seal1 = IntegrityInterface_seal(IIFace0, 
                                            secretsize1,
                                            (byte *)secret1,
                                            &blobsize1,
                                            &blob1);

    LOGd(DBG_TEST, "res_seal1(%d) -- blobsize1(%d)", res_seal1, blobsize1);

    /* Unseal secret. */
    unsigned char *data_buf;
    int datasize1;

    int res_unseal1 = IntegrityInterface_unseal(IIFace0, 
                                                blobsize1,
                                                blob1,
                                                &datasize1,
                                                &data_buf);

    lyon_sealed_data_t *s = (lyon_sealed_data_t *)data_buf;
      
    LOGd(DBG_TEST, "res_unseal1(%d) -- data1(%s) -- nonce(%s)", 
         res_unseal1, 
         &s->data[s->info.data_offset],
         &s->data[0]);
  
    /* Test quote and extend operations. */
    int res_quote_extend = test_quote_and_extend(IIFace0);

    LOGd(DBG_TEST, "res_quote_extend(%d)", res_quote_extend);

    if(DBG_TEST)
        DBG_iifacemap(3);
    
    /* Free the IntegrityInterface instance IIFace0. */
    int res_iif0_free = IntegrityInterface_free(IIFace0);
    LOGd(DBG_TEST, "res_iif0_free(%d) with IIFace0(%d), ref(%p) stored at(%p)",  
         res_iif0_free,
         *((int*)IIFace0),
         (int*)IIFace0,
         &IIFace0);
    
    if(DBG_TEST)
        DBG_iifacemap(4);
 
    return 0;
}

int main(int argc, char** argv)
{
    LOGd(DBG_TEST, "Starting bmsi_test.\n"); 

    LOGd(DBG_TEST, "\n***********************\n***** test_libb() *****\n***********************\n");
    test_libb();
    
    LOGd(DBG_TEST, "\n***********************\n***** test_libu() *****\n***********************\n");
    test_libu();
    
    LOGd(DBG_TEST, "End bmsi_test.\n"); 

    return 0;
}

