#include "local.h"

/*** some important global variables / functions ***/

#include <l4/log/l4log.h>
#include <linux/config.h>

/*** arch/../kernel/smpboot.c ***/

#include <linux/threads.h>

int smp_num_cpus = NR_CPUS;

/*** arch/../mm/init.c (needed by net/ipv4/inetpeer.c) ***
 *
 * This function is only used to dimension the "inet_peer_cache".
 * The only needed information is sysinfo->totalram - so we supply 
 * a dummy value.
 */

/* XXX maybe someone knows the difference between this and num_physpages ? */

#include <linux/mm.h>

void si_meminfo(struct sysinfo *val);
void si_meminfo(struct sysinfo *val)
{
	static int cnt = 0;
	LOG_Enter("(%i)", ++cnt);
	val->totalram = (8192 * 1024) / PAGE_SIZE;
}

/*** drivers/char/random.c ***/

/* XXX add more stuff from random.c on demand; maybe get the whole file and
 * clean it up */

#include <linux/random.h>

#define ENTROPY_LEN    (64*20)

static char entropy[ENTROPY_LEN] =
	"raiw8ShubiuC0vooniej4naWzah6jooRPhi0nahfroh1BaijWaiqu0soFie6hiex"
	"tee5fooBWiepie5rDoo8nohnbe3noSohbaisa5WiHaig7xieNohae7dushoap6Ke"
	"Shiwie4pTa0geijuFielaew6ceeMah4cquieFah7kik1RievFoor8boohach7Joh"
	"Vuquoh2klai3rohGphooNis0xoh2Theerah3FiejSheaze6hku5Pusaica8Dipha"
	"phahv4CaCeihiqu3See5rahvzo4deeHebie1DohjFahr0yieRoen3shuteic8Giu"
	"liec1ZohzaiD0thaFuth1quunieweR7nFaing2shJoh2susoweePi5toChoxael6"
	"Raes1jaeWad8daipvae8Quibcoh3XaetHoh3doukXuemohc8Ja5keileGeeth7za"
	"koh0RiitNee3mailchouD1jigodo2vuVToo2loduYuqui6kiCo1meigaga5Zahje"
	"Xeit2xeiphe7sohHhiif4Gaisoo7Jaekbeib4Caeche3Coilnu1WaimucheeC8pe"
	"meyi7Quesai7Leihpukei7YikaiShie5Soong7ronoh1goHoxighohW4fuiCi2ru"
	"Joo6pahgDa7vaiyojo3XovohtooxaW7pvayooPh0Tha7riemwohB7suithor3Zai"
	"shahX7juGoh5cuneMau7chaiwa2LoziekouBah0fchi7Fahsgohch2CuKekai1mo"
	"Pheipe2hdoo8nahZHeozo0foZavei5nejie8daiWvoh6dooZNoucoh4cyei3tieG"
	"xaiGae4lkaiy2Chayahkap0Bthaiph0KFee1rahdNais2yukPiopie4hKie0cana"
	"Gooz7quaMie3kapoChae6fupgohL0choDiecu3deVoo6thowmee0LohvDohn6jez"
	"Hie1naecBe3hiecuviad6GeaRo5thuorFaebee5cphie1BiychieLae1Yeetae8p"
	"xi5ZeivusahG5vooheik8SaeXegh0nieNahpoh0rPeighol8Geich4ciChai8daf"
	"Yaiwii6ryee0Jievvichee8GReekii2mxtio8heehae1Giinsoyen5KaphouXea2"
	"ge8mahGifeSh0guePeu2feetSheiyee1Being4ciPeazek6vbahvo8Thpa4faCag"
	"vahYo3buthia2DahVua5fookChiech7pCohjoo4fzooTo6kuQuojap6pZeireef4";
static int entropy_cnt;

/* Hmm, Norman just look at this ... I'm not that good at programming */

/* ENTROPY POOL
 * +-------------------------------------------------------------+
 * |                |\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\|
 * |/////////////////////////////////////////////////////////////|
 * |/////////////////////////////////////////////////////////////|
 * |XXXXXXXXXXXXXXXXXXXXXXXXXX|                                  |
 * +-------------------------------------------------------------+
 *                  ^         ^
 *                  |         |
 *                  |     next pos
 *             current pos <entropy_cnt>
 *
 * We want to use "entropy" from pos 0 in buffer ascending. At the end we wrap
 * around. So I identified 3 "ranges":
 *
 * A |\\\\|       ENTROPY_LEN - entropy_cnt ("- nbytes" if not wrapped)
 * B |////|  n * ((nbytes - A) / ENTROPY_LEN)
 * C |XXXX|       (nbytes - A) % ENTROPY_LEN
 */

void get_random_bytes(void *buf, int nbytes)
{
	int cnt = ENTROPY_LEN - entropy_cnt;
	char *cur = &entropy[entropy_cnt];

#ifdef DEBUG_LIBLX
	if (nbytes < 0) {
		LOG_printf("Warning: nbytes < 0\n");
		return;
	}
#endif

	/* pre-compute new index into pool */
	entropy_cnt = (entropy_cnt + nbytes) % ENTROPY_LEN;

	/* range A */
	cnt = min(nbytes, cnt);
	memcpy(buf, cur, cnt);
	nbytes -= cnt;
	buf += cnt;

	/* range B */
	while (nbytes >= ENTROPY_LEN) {
		memcpy(buf, entropy, ENTROPY_LEN);
		nbytes -= ENTROPY_LEN;
		buf += ENTROPY_LEN;
	}

	/* range C */
	if (nbytes > 0)
		memcpy(buf, entropy, nbytes);
}

/*
 * TCP initial sequence number picking.  This uses the random number
 * generator to pick an initial secret value.  This value is hashed
 * along with the TCP endpoint information to provide a unique
 * starting point for each pair of TCP endpoints.  This defeats
 * attacks which rely on guessing the initial TCP sequence number.
 * This algorithm was suggested by Steve Bellovin.
 *
 * Using a very strong hash was taking an appreciable amount of the total
 * TCP connection establishment time, so this is a weaker hash,
 * compensated for by changing the secret periodically.
 */

/* F, G and H are basic MD4 functions: selection, majority, parity */
#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) (((x) & (y)) + (((x) ^ (y)) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

/*
 * The generic round function.  The application is so specific that
 * we don't bother protecting all the arguments with parens, as is generally
 * good macro practice, in favor of extra legibility.
 * Rotation is separate from addition to prevent recomputation
 */
#define ROUND(f, a, b, c, d, x, s)      \
        (a += f(b, c, d) + x, a = (a << s) | (a >> (32-s)))
#define K1 0
#define K2 013240474631UL
#define K3 015666365641UL

  /*
   * Basic cut-down MD4 transform.  Returns only 32 bits of result.
   */
static __u32 halfMD4Transform(__u32 const buf[4], __u32 const in[8])
{
	__u32 a = buf[0], b = buf[1], c = buf[2], d = buf[3];

	/* Round 1 */
	ROUND(F, a, b, c, d, in[0] + K1, 3);
	ROUND(F, d, a, b, c, in[1] + K1, 7);
	ROUND(F, c, d, a, b, in[2] + K1, 11);
	ROUND(F, b, c, d, a, in[3] + K1, 19);
	ROUND(F, a, b, c, d, in[4] + K1, 3);
	ROUND(F, d, a, b, c, in[5] + K1, 7);
	ROUND(F, c, d, a, b, in[6] + K1, 11);
	ROUND(F, b, c, d, a, in[7] + K1, 19);

	/* Round 2 */
	ROUND(G, a, b, c, d, in[1] + K2, 3);
	ROUND(G, d, a, b, c, in[3] + K2, 5);
	ROUND(G, c, d, a, b, in[5] + K2, 9);
	ROUND(G, b, c, d, a, in[7] + K2, 13);
	ROUND(G, a, b, c, d, in[0] + K2, 3);
	ROUND(G, d, a, b, c, in[2] + K2, 5);
	ROUND(G, c, d, a, b, in[4] + K2, 9);
	ROUND(G, b, c, d, a, in[6] + K2, 13);

	/* Round 3 */
	ROUND(H, a, b, c, d, in[3] + K3, 3);
	ROUND(H, d, a, b, c, in[7] + K3, 9);
	ROUND(H, c, d, a, b, in[2] + K3, 11);
	ROUND(H, b, c, d, a, in[6] + K3, 15);
	ROUND(H, a, b, c, d, in[1] + K3, 3);
	ROUND(H, d, a, b, c, in[5] + K3, 9);
	ROUND(H, c, d, a, b, in[0] + K3, 11);
	ROUND(H, b, c, d, a, in[4] + K3, 15);

	return buf[1] + b;          /* "most hashed" word */
	/* Alternative: return sum of all words? */
}

#undef ROUND
#undef F
#undef G
#undef H
#undef K1
#undef K2
#undef K3

/* This should not be decreased so low that ISNs wrap too fast. */
#define REKEY_INTERVAL 300
#define HASH_BITS      24

__u32 secure_tcp_sequence_number(__u32 saddr, __u32 daddr,
                                 __u16 sport, __u16 dport)
{
	static __u32 rekey_time;
	static __u32 count;
	static __u32 secret[12];
	struct timeval tv;
	__u32 seq;

	/*
	 * Pick a random secret every REKEY_INTERVAL seconds.
	 */
	do_gettimeofday(&tv);       /* We need the usecs below... */

	if (!rekey_time || (tv.tv_sec - rekey_time) > REKEY_INTERVAL) {
		rekey_time = tv.tv_sec;
		/* First three words are overwritten below. */
		get_random_bytes(&secret[3], sizeof(secret) - 12);
		count = (tv.tv_sec / REKEY_INTERVAL) << HASH_BITS;
	}

	/*
	 *  Pick a unique starting offset for each TCP connection endpoints
	 *  (saddr, daddr, sport, dport).
	 *  Note that the words are placed into the first words to be
	 *  mixed in with the halfMD4.  This is because the starting
	 *  vector is also a random secret (at secret+8), and further
	 *  hashing fixed data into it isn't going to improve anything,
	 *  so we should get started with the variable data.
	 */
	secret[0] = saddr;
	secret[1] = daddr;
	secret[2] = (sport << 16) + dport;

	seq = (halfMD4Transform(secret + 8, secret) &
	        ((1 << HASH_BITS) - 1)) + count;

	/*
	 *      As close as possible to RFC 793, which
	 *      suggests using a 250 kHz clock.
	 *      Further reading shows this assumes 2 Mb/s networks.
	 *      For 10 Mb/s Ethernet, a 1 MHz clock is appropriate.
	 *      That's funny, Linux has one built in!  Use it!
	 *      (Networks are faster now - should this be increased?)
	 */
	seq += tv.tv_usec + tv.tv_sec * 1000000;
#if 0
	printk("init_seq(%lx, %lx, %d, %d) = %d\n",
	        saddr, daddr, sport, dport, seq);
#endif
	return seq;
}

__u32 secure_ip_id(__u32 daddr)
{
	static time_t rekey_time;
	static __u32 secret[12];
	time_t t;

	t = CURRENT_TIME;
	if (!rekey_time || (t - rekey_time) > REKEY_INTERVAL) {
		rekey_time = t;
		/* First word is overwritten below. */
		get_random_bytes(secret + 1, sizeof(secret) - 4);
	}

	secret[0] = daddr;

	return halfMD4Transform(secret + 8, secret);
}

/*** drivers/net/Space.c ***/

struct net_device loopback_dev = {
	name:"lo",
	init:liblinux_lo_init
};

struct net_device *dev_base = &loopback_dev;
rwlock_t dev_base_lock = RW_LOCK_UNLOCKED;

/*** kernel/kmod.c ***/

void dev_probe_lock(void)
{
}
void dev_probe_unlock(void)
{
}

/*** kernel/signal.c (needed by tcp.c) ***/

struct task_struct;

int send_sig(int, struct task_struct *, int);
int send_sig(int sig, struct task_struct *p, int priv)
{
	static int cnt = 0;
	LOG_Enter("sig=%d (%i)", sig, ++cnt);
	return 0;
}

/*** kernel/signal.c (needed by tcp_input.c) ***/

#include <linux/types.h>

int kill_proc(pid_t pid, int sig, int priv);
int kill_proc(pid_t pid, int sig, int priv)
{
	static int cnt = 0;
	LOG_Enter("(%i)", ++cnt);
	return 0;
}

int kill_pg(pid_t pgrp, int sig, int priv);
int kill_pg(pid_t pgrp, int sig, int priv)
{
	static int cnt = 0;
	LOG_Enter("(%i)", ++cnt);
	return 0;
}

/*** kernel/softirq.c (needed by net/core/dev.c) ***
 *
 * bh_task_vec[] is needed for "old protocols, which are not threaded well or
 * which do not understand shared skbs". These indicate "old" state via 
 * "struct packet_type pt->data = 0":
 *
 * 802/p8022.c
 * ax25/af_ax25.c
 * econet/af_econet.c
 * ...
 *
 * Our protocols are "new" ones.
 */

struct tasklet_struct bh_task_vec[32];

/*** replacement for init/version.c, required by net/ipv4/ipconfig.c ***/ 
#include <linux/uts.h>
#include <linux/utsname.h>
#include <linux/version.h>

#define UTS_VERSION __DATE__ __TIME__

struct new_utsname system_utsname = { UTS_SYSNAME, UTS_NODENAME, UTS_RELEASE, UTS_VERSION,
	        UTS_MACHINE, UTS_DOMAINNAME
};


/*** lib/vsprintf.c ***/

#include <stdlib.h>

unsigned long simple_strtoul(const char *nptr, char **endptr, unsigned int base)
{
	return strtoul(nptr, endptr, base);
}

long simple_strtol(const char *nptr, char **endptr, unsigned int base)
{
	return strtol(nptr, endptr, base);
}
