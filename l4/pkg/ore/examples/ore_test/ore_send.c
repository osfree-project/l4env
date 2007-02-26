#include <stdlib.h>
#include <string.h>

#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/ore/ore.h>
#include <l4/sys/ipc.h>
#include <l4/dm_generic/types.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/l4rm/l4rm.h>

#if 0
#include <net/if_ether.h>
#else
#define ETH_HLEN 14
#endif

#define DS_SIZE 1024*1024

#define ERRLOG(err) { if((err)) { \
			LOG_Error("%d (%s)", (err), l4env_errstr((-err)));	\
			return err; }}

#define TEST_OPEN       0
#define TEST_SEND       1
#define TEST_RECV       0

char LOG_tag[9] = "ore_test";

l4_ssize_t l4libc_heapsize = 64 * 1024;

/* Colorful way to find test messages in LOG output. */
void testlog(char *s);
void testlog(char *s)
{
    LOG("\033[32m%s\033[0m",s);
}

void test_open(void);
void test_open(void)
{
    l4ore_config conf           = L4ORE_DEFAULT_CONFIG;
    l4dm_dataspace_t send_ds    = L4DM_INVALID_DATASPACE;
    l4dm_dataspace_t recv_ds    = L4DM_INVALID_DATASPACE;
    int handle=-1;
    int ret=-1;
    unsigned char mac[6];

    testlog("testing open(\"lo\") for shared memory");

    ret = l4dm_mem_open(L4DM_DEFAULT_DSM, DS_SIZE, 0,
            L4DM_CONTIGUOUS, "client send dataspace", &send_ds);
    LOG("created send DS: %d", ret);
    
    ret = l4dm_mem_open(L4DM_DEFAULT_DSM, DS_SIZE, 0,
            L4DM_CONTIGUOUS, "client recv dataspace", &recv_ds);
    LOG("created recv DS: %d", ret);

    testlog("opening with shared send ds");
    conf.ro_send_ds = send_ds;
    conf.ro_recv_ds = L4DM_INVALID_DATASPACE;
    handle = l4ore_open("lo", mac, &conf);
    LOG("Opened handle %d", handle);
    LOG_MAC(1,mac);

    testlog("opening with shared recv ds");
    conf.ro_send_ds = L4DM_INVALID_DATASPACE;
    conf.ro_recv_ds = recv_ds;
    
    testlog("opening with shared send and recv ds");
    conf.ro_send_ds = send_ds;
    conf.ro_recv_ds = recv_ds;
}

void test_send(void);
void test_send(void)
{
    l4ore_config conf           = L4ORE_DEFAULT_CONFIG;
    l4dm_dataspace_t send_ds    = L4DM_INVALID_DATASPACE;
    int handle=-1;
    int ret=-1;
    int i,j;
    unsigned char mac[6];
    void *addr;

    testlog("testing send() through shared memory.");
    
    ret = l4dm_mem_open(L4DM_DEFAULT_DSM, DS_SIZE, 0,
            L4DM_CONTIGUOUS, "client send dataspace", &send_ds);
    LOG("created send DS: %d", ret);
    
    testlog("opening");
    conf.ro_send_ds = send_ds;
    handle = l4ore_open("lo", mac, &conf);

    addr = l4ore_get_send_area(handle);
    testlog("going sending");
    for (i=0; i<5; i++)
    {
        for (j=0; j<100; j++)
            *(int *)(addr + j*sizeof(int)) = i*j;
        l4ore_send(handle, addr, 100*sizeof(int));
        addr += (100*sizeof(int));
    }
}

int main(int argc, char **argv)
{
    LOG("Hello from the ORe client");

#if TEST_OPEN
    test_open();
#endif
#if TEST_SEND
    test_send();
#endif

    LOG("Finished. Going to sleep.");
    l4_sleep_forever();
    return 0;
}
