/*
 * \brief   Lyon test applicacation.
 * \date    2006-08-25
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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

/* L4-specific includes */
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/lyon/lyon.h>

/*
 * ***************************************************************************
 */

int l4libc_heapsize = 64*1024;

/*
 * ***************************************************************************
 */

const static lyon_hash_t seal_hash = "seal_test_hash01234\0";

/*
 * ***************************************************************************
 */

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
        printf(format, (unsigned char) data[i]);
        if (i % 20 == 19 && i + 1 != len)
            printf("\n%s", prefix);
    }
    printf("\n");
}

/*
 * ***************************************************************************
 */

static int
test_seal_unseal(const lyon_id_t id)
{
    //const char *secret = "This is very secret data that must be "
    //                     "protedted by sealed memory!";
    const char *secret = "This is very secret data that must be "
                         "protected by sealed memory!\n";
    char data[1024];
    char unsealed_data[1024];
    int  ret;

    ret = lyon_seal(L4_INVALID_ID, id, sizeof(seal_hash), seal_hash,
                    strlen(secret) + 1, secret, sizeof(data), data);
    printf("lyon_seal(): %d\n", ret);
    print_bytes("seal: ", "%02x ", data, ret);

    if (ret > 0)
    {
        ret = lyon_unseal(L4_INVALID_ID, ret, data, sizeof(unsealed_data), unsealed_data);
        printf("lyon_unseal(): %d\n", ret);
        if (ret > 0)
        {
            lyon_sealed_data_t *s = (lyon_sealed_data_t *) unsealed_data;
            print_bytes("unseal: ", "%02x ", unsealed_data, ret);
            /* we know that both the nonce and the data are actually
             * null-terminated strings */
            printf("offset=%u, length=%u: nonce='%s'\ndata='%s'",
                   s->info.data_offset, s->info.data_len,
                   &s->data[0], &s->data[s->info.data_offset]);
        }
    }

    return ret;
}


static int
test_quote(lyon_id_t my_id_out)
{
    lyon_quote_t quote;
    char        *nonce = "nonce";
    int          ret;

    ret = lyon_quote(L4_INVALID_ID, strlen(nonce), nonce, &quote);

    print_result("lyon_quote()", ret);
    print_bytes("my_id: ", "%02x ", quote.data.id, sizeof(quote.data.id));
    print_bytes("my_hash: ", "%02x ", quote.data.hash, sizeof(quote.data.hash));

    memcpy(my_id_out, quote.data.id, sizeof(lyon_id_t));

    return ret;
}

static int
test_extend()
{
    lyon_quote_t quote;
    char        *nonce = "nonce";
    int          ret;

    lyon_id_t id1, id2, id3;
    lyon_hash_t hash1, hash2, hash3;

    /* Quote initial id and data. */
    ret = lyon_quote(L4_INVALID_ID, strlen(nonce), nonce, &quote);
    memcpy(id1, quote.data.id, sizeof(lyon_id_t));
    memcpy(hash1, quote.data.hash, sizeof(lyon_hash_t));

    print_result("lyon_quote() 1: ", ret);
    print_bytes("id_1   : ", "%02x ", id1, sizeof(id1));
    print_bytes("hash_1 : ", "%02x ", hash1, sizeof(hash1));
    
    /* Quote again and check id and data. */
    ret = lyon_quote(L4_INVALID_ID, strlen(nonce), nonce, &quote);
    memcpy(id2, quote.data.id, sizeof(lyon_id_t));
    memcpy(hash2, quote.data.hash, sizeof(lyon_hash_t));

    print_result("lyon_quote() 2: ", ret);
    print_bytes("id_2  : ", "%02x ", id2, sizeof(id2));
    print_bytes("hash_2: ", "%02x ", hash2, sizeof(hash2));
    
    print_result("id_1 == id_2?: ",
                 memcmp(id1, id2, sizeof(lyon_id_t)));
    
    print_result("hash_1 == hash_2?: ",
                 memcmp(hash1, hash2, sizeof(lyon_hash_t)));

    /* Extend data. */
    const char *data = "my_data";
    unsigned int datalen = strlen(data);
    l4_threadid_t me = l4_myself();
    
    ret = lyon_extend(me, datalen, data);
    print_result("lyon_extend(): ", ret);

    /* Quote and check id and data. */
    ret = lyon_quote(L4_INVALID_ID, strlen(nonce), nonce, &quote);
    memcpy(id3, quote.data.id, sizeof(lyon_id_t));
    memcpy(hash3, quote.data.hash, sizeof(lyon_hash_t));

    print_result("lyon_quote() 3: ", ret);
    print_bytes("id_3  : ", "%02x ", id3, sizeof(id3));
    print_bytes("hash_3: ", "%02x ", hash3, sizeof(hash3));
    
    print_result("id_2 == id_3?: ",
                 memcmp(id2, id3, sizeof(lyon_id_t)));
    
    print_result("hash_2 != hash_3?: ",
                 !memcmp(hash2, hash3, sizeof(lyon_hash_t)));

    return ret;
}

/*
 * ***************************************************************************
 */

int
main(int argc, char **argv)
{

    int ret;	
    l4_threadid_t me     = l4_myself();
    l4_threadid_t child1 = me;  
    l4_threadid_t child2 = me; 
    l4_threadid_t child3 = me; 
    lyon_id_t     my_id  = "new_lyon_id_0_01234";
    lyon_id_t     new1   = "new_lyon_id_1_01234";
    lyon_id_t     new2   = "new_lyon_id_2_01234";
    lyon_id_t     new3   = "new_lyon_id_3_01234";
    lyon_id_t     my_id_from_quote;
    char          dummy_hash[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    child1.id.task += 1;
    child2.id.task += 2;
    child3.id.task += 3;

    printf("lyontest start!\n");

#if 1
    ret = lyon_add(me, lyon_nil_id, me, my_id, "ROOT NODE LYONTEST", dummy_hash);
    print_result("lyon_add() 1", ret);
#endif

#if 1
    ret = lyon_add(me, my_id, child1, new1, "test1", dummy_hash);
    print_result("lyon_add() 1", ret);
#endif

#if 1
    ret = lyon_add(me, my_id, child2, new2, "test2", seal_hash);
    print_result("lyon_add() 2", ret);
#endif

#if 1
    ret = lyon_add(me, new2, child3, new3, "test3", dummy_hash);
    print_result("lyon_add() 3", ret);
#endif

#if 0
    ret = lyon_count();
    assert(ret == 4);
#endif

#if 1
    ret = lyon_del(me, new1);
    print_result("lyon_del()", ret);
#endif

#if 0
    ret = lyon_count();	
    assert(ret == 3);
#endif

#if 1
    ret = test_quote(my_id_from_quote);
    print_result("my_id == my_id_from_quote?: ",
                 memcmp(my_id, my_id_from_quote, sizeof(lyon_id_t)));
#endif

#if 1
    ret = test_seal_unseal(my_id_from_quote);
#endif

#if 1
    ret = test_extend();
#endif

    printf("\nlyontest done!\n");

    return 0;
}
