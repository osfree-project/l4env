#include <stdlib.h>
#include <string.h>

#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/ore/ore.h>

#if 0
#include <net/if_ether.h>
#else
#define ETH_HLEN 14
#endif

#define DS_SIZE 1024*1024

#define ERRLOG(err) { if((err)) { \
			LOG_Error("%d (%s)", (err), l4env_errstr((-err)));	\
			return err; }}

char LOG_tag[9] = "ore_test";

l4_ssize_t l4libc_heapsize = 64 * 1024;

/* test open() features */
#define TEST_OPEN           0
/* test send/receive through loopback */
#define TEST_LOOPBACK	    0
/* test send/receive via eth0 */
#define TEST_ETH0           0
/* test assignment of original device MAC */
#define TEST_DEVICE_MAC     0
/* test reading / writing configuration */
#define TEST_CONFIG         1

/* Colorful way to find test messages in LOG output. */
void testlog(char *s);
void testlog(char *s)
{
  LOG("\033[32m%s\033[0m",s);
}

/* Test several features of opening a connection to ORe.
 *
 * 1. Test opening the loopback device.         ==> Succeeds.
 * 2. Test opening the eth0 device.             ==> Succeeds.
 * 3. Test opening an invalid device 'bla'	==> Returns error.
 * 4. Test opening a device with a NULL pointer for the MAC address.
 *	==> Fails. In the future this should
 *	be changed, so that the client
 *	library takes care of allocating
 *	memory for the MAC address.
 */
void test_open(void);
void test_open(void)
{
  unsigned char mac[6];
  char *null_mac = NULL;
  int id = 0;
  int i;
  l4ore_config ore_conf = L4ORE_DEFAULT_CONFIG;

  LOG("Testing open()");

  LOG("&mac = %p", &(mac));
  LOG("*mac = %p", mac);

  mac[0] = mac[1] = mac[2] = mac[3] = mac[4] = mac[5] = 0;

  for (i = 0; i < 6; i++)
    LOG("mac[%d] = %d", i, mac[i]);

  // loopback
  testlog("1. Opening loopback device. Expect success.");
  LOG("MAC = %02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  id = l4ore_open("lo", mac, NULL, NULL, &ore_conf);
  LOG("ORe returned id: %d", id);
  LOG("MAC = %02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  mac[0] = mac[1] = mac[2] = mac[3] = mac[4] = mac[5] = 0;

  // eth0
  testlog("2. Opening device eth0. Expect success.");
  LOG("MAC = %02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  id = l4ore_open("eth0", mac, NULL, NULL, &ore_conf);
  LOG("ORe returned id: %d", id);
  LOG("MAC = %02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  mac[0] = mac[1] = mac[2] = mac[3] = mac[4] = mac[5] = 0;

  // invalid device name
  testlog("3. Opening invalid device name 'bla'. Expect invalid handle.");
  LOG("MAC = %02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  id = l4ore_open("bla", mac, NULL, NULL, &ore_conf);
  LOG("ORe returned id: %d", id);
  LOG("MAC = %02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  mac[0] = mac[1] = mac[2] = mac[3] = mac[4] = mac[5] = 0;

  // invalid device name
  testlog("4. Opening device eth0 with no mac allocated. Fails. :(");
  LOG("null_mac = %p", null_mac);
  id = l4ore_open("eth0", null_mac, NULL, NULL, &ore_conf);
  LOG("ORe returned id: %d", id);
  LOG("null_mac = %p", null_mac);
  if (null_mac)
    LOG("MAC = %02x:%02x:%02x:%02x:%02x:%02x", null_mac[0], null_mac[1],
	null_mac[2], null_mac[3], null_mac[4], null_mac[5]);
}

/* Test sending and receiving data through a given device. */
void test_send(char *dev);
void test_send(char *dev)
{
  unsigned char buf[256];
  unsigned char buf3[256];
  char *buf2 = malloc(300);
  unsigned char mac[6];
  int i, channel, size;
  l4ore_config ore_conf = L4ORE_DEFAULT_CONFIG;

  for (i=ETH_HLEN; i<256; i++)
    {
      buf[i] = i;
      buf3[i] = 256 - i;
    }

  channel = l4ore_open(dev, mac, NULL, NULL, &ore_conf);
  LOG("MAC = %02X:%02X:%02X:%02X:%02X:%02X", mac[0],
      mac[1], mac[2], mac[3], mac[4], mac[5]);
  LOG("opened '%s' with channel: %d", dev, channel);

  /*
   * SRC = DEST       = MAC
   */
  buf[0] = buf[6]  = mac[0];
  buf[1] = buf[7]  = mac[1];
  buf[2] = buf[8]  = mac[2];
  buf[3] = buf[9]  = mac[3];
  buf[4] = buf[10] = mac[4];
  buf[5] = buf[11] = mac[5];

  buf3[0] = buf3[6]  = mac[0];
  buf3[1] = buf3[7]  = mac[1];
  buf3[2] = buf3[8]  = mac[2];
  buf3[3] = buf3[9]  = mac[3];
  buf3[4] = buf3[10] = mac[4];
  buf3[5] = buf3[11] = mac[5];

  // length field ==> 256 bytes
  buf[12] = 0x01;
  buf[13] = 0x00;

  LOG("buf1 content: '%02x %02x %02x %02x %02x %02x %02x "
                     "%02x %02x %02x %02x %02x...'",
      buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
      buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);

  LOG("'...%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x'",
      buf[12], buf[13], buf[14], buf[15], buf[16], buf[17],
      buf[18], buf[19], buf[20], buf[21], buf[22], buf[23]);

  LOG("buf3 content: '%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x...'", 
      buf3[0], buf3[1], buf3[2], buf3[3], buf3[4], buf3[5],
      buf3[6], buf3[7], buf3[8], buf3[9], buf3[10], buf3[11]);

  LOG("'...%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x'",
      buf3[12], buf3[13], buf3[14], buf3[15], buf3[16], buf3[17],
      buf3[18], buf3[19], buf3[20], buf3[21], buf3[22], buf3[23]);

  LOG("now sending buf");
  i = l4ore_send(channel, buf, 256);
  LOG("sent buf: %d", i);

  LOG("now sending buf3");
  i = l4ore_send(channel, buf3, 256);
  LOG("sent buf3: %d", i);

  size = 300;
  LOG("set rx buffer size");

  i = l4ore_recv_blocking(channel, &buf2, &size);
  LOG("received packet: %d, size = %d", i, size);
  LOG("rx buf content: '%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x...'",
      buf2[0], buf2[1], buf2[2], buf2[3], buf2[4], buf2[5],
      buf2[6], buf2[7], buf2[8], buf2[9], buf2[10], buf2[11]);

  LOG("'...%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x'",
      buf2[12], buf2[13], buf2[14], buf2[15], buf2[16], buf2[17],
      buf2[18], buf2[19], buf2[20], buf2[21], buf2[22], buf2[23]);

  i = l4ore_recv_blocking(channel, &buf2, &size);
  LOG("received packet: %d, size = %d", i, size);
  LOG("rx buf content: '%02x %02x %02x %02x %02x %02x %02x %02x "
      "%02x %02x %02x %02x...'",
      buf2[0], buf2[1], buf2[2], buf2[3], buf2[4], buf2[5],
      buf2[6], buf2[7], buf2[8], buf2[9], buf2[10], buf2[11]);
  LOG("'...%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x'",
      buf2[12], buf2[13], buf2[14], buf2[15], buf2[16], buf2[17],
      buf2[18], buf2[19], buf2[20], buf2[21], buf2[22], buf2[23]);
}

/* Test opening several connections with the KEEP_DEVICE_MAC switch
 * --> only the first connection will receive the real device MAC.
 */
void test_original_mac(void);
void test_original_mac(void)
{
  unsigned char mac[6];
  unsigned char mac2[6];
  unsigned char mac3[6];  
  
  int id    = 0;
  int id2   = 0;
  int id3   = 0;

  l4ore_config ore_conf = L4ORE_DEFAULT_CONFIG;
  ore_conf.ro_keep_device_mac = 1;

  LOG("Testing open()");

  memset(mac, 0, 6);
  memset(mac2, 0, 6);
  memset(mac3, 0, 6);

  LOG("MAC1 = %02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  LOG("MAC2 = %02x:%02x:%02x:%02x:%02x:%02x",
      mac2[0], mac2[1], mac2[2], mac2[3], mac2[4], mac2[5]);
  LOG("MAC3 = %02x:%02x:%02x:%02x:%02x:%02x",
      mac3[0], mac3[1], mac3[2], mac3[3], mac3[4], mac3[5]);

  // open eth0 three times
  testlog("Opening eth0 three times.");
  testlog("Expect first only conn to receive device MAC.");

  id    = l4ore_open("eth0", mac, NULL, NULL, &ore_conf);
  id2   = l4ore_open("eth0", mac2, NULL, NULL, &ore_conf);
  id3   = l4ore_open("eth0", mac3, NULL, NULL, &ore_conf);
  
  LOG("ORe ids are: %d, %d, %d", id, id2, id3);
  LOG("MAC1 = %02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  LOG("MAC2 = %02x:%02x:%02x:%02x:%02x:%02x",
      mac2[0], mac2[1], mac2[2], mac2[3], mac2[4], mac2[5]);
  LOG("MAC3 = %02x:%02x:%02x:%02x:%02x:%02x",
      mac3[0], mac3[1], mac3[2], mac3[3], mac3[4], mac3[5]);
}

void test_config(void);
void test_config(void)
{
  unsigned char mac[6];
  int id, id2;
  l4ore_config ore_conf = L4ORE_DEFAULT_CONFIG;
  ore_conf.ro_keep_device_mac = 1;

  testlog("testing device configuration. initial config:");
  LOG_CONFIG(ore_conf);

  testlog("opening eth0 with keep_device_mac");
  id = l4ore_open("eth0", mac, NULL, NULL, &ore_conf);

  LOG("MAC = %02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  testlog("configuration after 1st open.");
  testlog("expect keep_device_mac == 1");
  testlog("expect IRQ and MTU filled in correctly.");
  LOG_CONFIG(ore_conf);

  testlog("opening eth0 with keep_device_mac (2nd time)");
  id2 = l4ore_open("eth0", mac, NULL, NULL, &ore_conf);
  LOG("MAC = %02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  testlog("configuration after 2nd open.");
  testlog("expect keep_device_mac == 0");
  LOG_CONFIG(ore_conf);
  testlog("done");
}

int main(int argc, char **argv)
{
  LOG("Hello from the ORe client");

#if TEST_OPEN
  test_open();
#endif

#if TEST_LOOPBACK
  test_send("lo");
#endif

#if TEST_ETH0
  /* This should test send/receive via the real hardware located at eth0.
   * Unfortunately ethernet cards cannot receive packets they sent themselves,
   * so just repeating the loopback test won't work.
   *
   * 1. Manual tests work:
   *		(a) Sending a packet from ORe via eth0 results in the packet being
   *			seen by tcpdump from another computer.
   *		(b) arping'ing an ORe MAC address results in ping packets being
   *			enqueued into the rx queue of eth0.
   *
   * 2. We need to test a real scenario. Next work will be put into a ORe driver
   *	  for tftp, so that this will probably be the scenario of choice.
   *
   * 3. TODO: ORe should directly deliver packets for own clients. Otherwise
   *          sending to one self or to local clients will not work.
   */
   test_send("eth0");
#endif

#if TEST_DEVICE_MAC
    test_original_mac();
#endif

#if TEST_CONFIG
    test_config();
#endif

  LOG("Finished. Going to sleep.");
  l4_sleep_forever();
  return 0;
}
