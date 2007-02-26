INTERFACE:

#include "types.h"



#define P4_PERF_BRANCHES         0
#define P4_PERF_BRANCHES_MISPR   1
#define P4_PERF_TC_DELIVER       2
#define P4_PERF_BPU_FETCH        3
#define P4_PERF_ITLB             4
#define P4_PERF_MEM_CANCEL       5
#define P4_PERF_MEM_COMPLETE     6
#define P4_PERF_LOAD_REPLAY      7
#define P4_PERF_STORE_REPLAY     8
#define P4_PERF_MOB_REPLAY       9
#define P4_PERF_PAGE_WALK       10
#define P4_PERF_L2CACHE         11
#define P4_PERF_BUS_TRANS       12
#define P4_PERF_FSB             13
#define P4_PERF_BUS_OP          14
#define P4_PERF_MACHINE_CLEAR   15
#define P4_PERF_FRONT_EVENT     16
#define P4_PERF_EXEC_EVENT      17
#define P4_PERF_REPLAY_EVENT    18
#define P4_PERF_INSTR_RETD      19
#define P4_PERF_UOP_RETD        20
#define P4_PERF_UOP_TYPE        21
#define P4_PERF_MP_BR_RETD      22

#define IA32_MISC_ENABLE		0x1a0
#define IA32_COUNTER_BASE		0x300
#define IA32_CCCR_BASE			0x360
#define IA32_TC_PRECISE_EVENT		0x3f0
#define IA32_PEBS_ENABLE		0x3f1
#define IA32_PEBS_MATRIX_VERT		0x3f2
#define IA32_DS_AREA			0x600
#define IA32_LER_FROM_LIP		0x1d7
#define IA32_LER_TO_LIP		        0x1d8
#define IA32_DEBUGCTL                   0x1d9
#define IA32_LASTBRANCH_TOS		0x1da
#define IA32_LASTBRANCH_0		0x1db
#define IA32_LASTBRANCH_1		0x1dc
#define IA32_LASTBRANCH_2		0x1dd
#define IA32_LASTBRANCH_3		0x1de

 
#define IA32_ENABLE_FAST_STRINGS	(1 << 0)
#define IA32_ENABLE_X87_FPU		(1 << 2)
#define IA32_ENABLE_THERMAL_MONITOR	(1 << 3)
#define IA32_ENABLE_SPLIT_LOCK_DISABLE	(1 << 4)
#define IA32_ENABLE_PERFMON		(1 << 7)
#define IA32_ENABLE_BRANCH_TRACE	(1 << 11)
#define IA32_ENABLE_PEBS		(1 << 12)

 
#define IA32_PEBS_REPLAY_TAG_MASK	((1 << 12)-1)
#define IA32_PEBS_UOP_TAG		(1 << 24)
#define IA32_PEBS_ENABLE_PEBS		(1 << 25)


struct ia32_ds_area_t
{
  unsigned bts_buffer_base;
  unsigned bts_index;
  unsigned bts_absolute_maximum;
  unsigned bts_interrupt_threshold;
  unsigned pebs_buffer_base;
  unsigned pebs_index;
  unsigned pebs_absolute_maximum;
  unsigned pebs_interrupt_threshold;
  long long pebs_counter_reset;
  unsigned reserved;
};

struct cccr_t
{
  unsigned rsvd0         : 12;
  unsigned enable        :  1;
  unsigned escr_select   :  3;
  unsigned rsvd1         :  2;
  unsigned compare       :  1;
  unsigned complement    :  1;
  unsigned threshold     :  4;
  unsigned edge          :  1;
  unsigned force_ovf     :  1;
  unsigned ovf_pmi       :  1;
  unsigned rsvd2         :  3;
  unsigned cascade       :  1;
  unsigned ovf           :  1;
};

struct escr_t
{
  unsigned rsvd0         :  2;
  unsigned usr           :  1;
  unsigned os            :  1;
  unsigned tag_enable    :  1;
  unsigned tag_value     :  4;
  unsigned event_mask    : 16;
  unsigned event_select  :  6;
  unsigned rsvd1         :  1;
};

#define WRMSR(ecx,eax,edx) ({unsigned d; asm volatile("wrmsr" : "=a"(d),"=d"(d),"=c"(d):\
						      "0"(eax),"1"(edx),"2"(ecx));})

#define WRMSR64(reg,val) ({asm volatile("wrmsr": :"c"(reg), "A"(val));})

#define RDMSR(ecx,eax,edx) ({unsigned d;asm volatile("rdmsr":"=a"(eax),"=d"(edx),"=c"(d):"2"(ecx));})


IMPLEMENTATION[perfp4]:


/*
 * Specification of the Pentium 4 events, their allowed counters, and
 * the valid event masks.
 *
 * Copied from x86_input.c of the L4-KA Kernel (Hazelnut).
 */







struct perfctr_t {
    byte_t	event_select;
    byte_t	escr_select;
    byte_t	ctr_low;
    byte_t	ctr_high;
    byte_t	special;
    char	*name;
    struct {
	byte_t	bitnum;
	char	*desc;
    } mask[9];
} __attribute__ ((packed));

static struct perfctr_t perf_counters[] = {
    /*
     * Non-Retirement events
     */

    { 0x06, 0x05, 12, 17, 0, "Branches", {
	{ 0, "Predicted NT" },
	{ 1, "Mispredicted NT" },
	{ 2, "Predicted taken" },
	{ 3, "Mispredicted taken" },
	{ 0, NULL }} },
    { 0x03, 0x04, 12, 17, 0, "Mispredicted branches", {
	{ 0, "Not bogus" },
	{ 0, NULL }} },
    { 0x01, 0x01, 4, 7, 0, "Trace cache deliver", {
	{ 2, "Delivering" },
	{ 5, "Building" },
	{ 0, NULL }} },
    { 0x03, 0x00, 0, 3, 0, "BPU fetch request", {
	{ 0, "TC lookup miss" },
	{ 0, NULL }} },
    { 0x18, 0x03, 0, 3, 0, "ITLB reference", {
	{ 0, "Hit" },
	{ 1, "Miss" },
	{ 2, "Uncachable hit" },
	{ 0, NULL }} },
    { 0x02, 0x05, 8, 11, 0, "Memory cancel", {
	{ 2, "Req buffer full" },
	{ 7, "Cache miss" },
	{ 0, NULL }} },
    { 0x08, 0x02, 8, 11, 0, "Memory complete", {
	{ 0, "Load split completed" },
	{ 1, "Split store completed" },
	{ 0, NULL }} },
    { 0x04, 0x02, 8, 11, 0, "Load port replay", {
	{ 1, "Split load" },
	{ 0, NULL }} },
    { 0x05, 0x02, 8, 11, 0, "Store port replay", {
	{ 1, "Split store" },
	{ 0, NULL }} },
    { 0x03, 0x02, 0, 3, 0, "MOB load replay", {
	{ 1, "Unknown store address" },
	{ 3, "Unknown store data" },
	{ 4, "Partial overlap" },
	{ 5, "Unaligned address" },
	{ 0, NULL }} },
    { 0x01, 0x04, 0, 3, 0, "Page walk type", {
	{ 0, "DTLB miss" },
	{ 1, "ITLB miss" },
	{ 0, NULL }} },
    { 0x0c, 0x07, 0, 3, 1, "L2 cache reference", {
	{ 0, "Read hit shared" },
	{ 1, "Read hit exclusive" },
	{ 2, "Read hit modified" },
	{ 8, "Read miss" },
	{ 10, "Write miss" },
	{ 0, NULL }} },
    { 0x03, 0x06, 0, 1, 1, "Bus transaction", {
	{ 0, "Req type (SET THIS)" },
	{ 5, "Read" },
	{ 6, "Write" },
	{ 7, "UC access" },
	{ 8, "WC access" },
	{ 13, "Own CPU stores" },
	{ 14, "Non-local access" },
	{ 15, "Prefetch" },
	{ 0, NULL }} },
    { 0x17, 0x06, 0, 3, 1, "FSB activity", {
	{ 0, "Ready drive" },
	{ 1, "Ready own" },
	{ 2, "Ready other" },
	{ 3, "Busy drive" },
	{ 4, "Busy own" },
	{ 5, "Busy other" },
	{ 0, NULL }} },
    { 0x05, 0x07, 0, 1, 2, "Bus operation", {
	/* Special mask handling. */
	{ 0, NULL }} },
    { 0x02, 0x05, 12, 17, 0, "Machine clear", {
	{ 0, "Any reason" },
	{ 2, "Memory ordering" },
	{ 0, NULL }} },

    /*
     * At-Retirement events.
     */

    { 0x08, 0x05, 12, 17, 3, "Front end event", {
	{ 0, "Not bogus" },
	{ 1, "Bogus" },
	{ 0, NULL }} },
    { 0x0c, 0x05, 12, 17, 4, "Execution event", {
	{ 0, "Not bogus 0" },
	{ 1, "Not bogus 1" },
	{ 2, "Not bogus 2" },
	{ 3, "Not bogus 3" },
	{ 4, "Bogus 0" },
	{ 5, "Bogus 1" },
	{ 6, "Bogus 2" },
	{ 7, "Bogus 3" },
	{ 0, NULL }} },
    { 0x09, 0x05, 12, 17, 5, "Replay event", {
	{ 0, "Not bogus" },
	{ 1, "Bogus" },
	{ 0, NULL }} },
    { 0x02, 0x04, 12, 17, 6, "Instr retired", {
	{ 0, "Non bogus untagged" },
	{ 1, "Non bogus tagged" },
	{ 2, "Bogus untagged" },	
	{ 3, "Bogus tagged" },
	{ 0, NULL }} },
    { 0x01, 0x04, 12, 17, 7, "Uops retired", {
	{ 0, "Not bogus" },
	{ 1, "Bogus" },
	  { 0, NULL }} },
    { 0x02, 0x02, 12, 17, 0, "Uop type", {
        { 1, "Tagloads"},
	{ 2, "Tagstores" },
	  { 0, NULL }} },
    { 0x05, 0x02,  4,  7, 0, "Mispredicted branch retired",{
        { 1, "Conditional"},
	{ 2, "Call"},
	{ 3, "Return"},
	{ 4, "Indirect"},
	{ 0, NULL}}}
};

struct replay_tag_t {
    uint16	pebs_enable;
    uint16	pebs_matrix;
    char 	*desc;
} __attribute__ ((packed));

static struct replay_tag_t replay_tags[] = {
    { 0x0001, 0x01, "L1 cache load miss" },
    { 0x0002, 0x01, "L2 cache load miss" },
    { 0x0004, 0x01, "DTLB load miss" },
    { 0x0004, 0x02, "DTLB store miss" },
    { 0x0004, 0x03, "DTLB all miss" },
    { 0x0100, 0x01, "MOB load" },
    { 0x0200, 0x01, "Split load" },
    { 0x0200, 0x02, "Split store" }
};


/*
 * Helper array for easily keeping track of which ESCR MSRs are in use.
 */

static word_t escr_msr[18];


/*
 * Translation table for ESCR numbers (as specified by cccr_select) to
 * ESCR addresses (add 0x300 to get actual address).  A zero value
 * indicates an invalid ESCR selector.
 */

static byte_t escr_to_addr[18][8] = {
    { 0xa0, 0xb4, 0xaa, 0xb6, 0xac, 0xc8, 0xa2, 0xa0 }, // Ctr 0
    { 0xb2, 0xb4, 0xaa, 0xb6, 0xac, 0xc8, 0xa2, 0xa0 }, // Ctr 1
    { 0xb3, 0xb5, 0xab, 0xb7, 0xad, 0xc9, 0xa3, 0xa1 }, // Ctr 2
    { 0xb3, 0xb5, 0xab, 0xb7, 0xad, 0xc9, 0xa3, 0xa1 }, // Ctr 3
    { 0xc0, 0xc4, 0xc2, 0x00, 0x00, 0x00, 0x00, 0x00 }, // Ctr 4
    { 0xc0, 0xc4, 0xc2, 0x00, 0x00, 0x00, 0x00, 0x00 }, // Ctr 5
    { 0xc1, 0xc5, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00 }, // Ctr 6
    { 0xc1, 0xc5, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00 }, // Ctr 7
    { 0xa6, 0xa4, 0xae, 0xb0, 0x00, 0xa8, 0x00, 0x00 }, // Ctr 8
    { 0xa6, 0xa4, 0xae, 0xb0, 0x00, 0xa8, 0x00, 0x00 }, // Ctr 9
    { 0xa7, 0xa5, 0xaf, 0xb1, 0x00, 0xa9, 0x00, 0x00 }, // Ctr 10
    { 0xa7, 0xa5, 0xaf, 0xb1, 0x00, 0xa9, 0x00, 0x00 }, // Ctr 11
    { 0xba, 0xca, 0xbe, 0xbc, 0xb8, 0xcc, 0xe0, 0x00 }, // Ctr 12
    { 0xba, 0xca, 0xbe, 0xbc, 0xb8, 0xcc, 0xe0, 0x00 }, // Ctr 13
    { 0xbb, 0xcb, 0xbd, 0x00, 0xb9, 0xcd, 0xe1, 0x00 }, // Ctr 14
    { 0xbb, 0xcb, 0xbd, 0x00, 0xb9, 0xcd, 0xe1, 0x00 }, // Ctr 15
    { 0xba, 0xca, 0xbe, 0xbc, 0xb8, 0xcc, 0xe0, 0x00 }, // Ctr 16
    { 0xbb, 0xcb, 0xbd, 0x00, 0xb9, 0xcd, 0xe1, 0x00 }  // Ctr 17
};

static inline  void wrmsr(const dword_t reg, const qword_t val)
{
    __asm__ __volatile__ (
	"wrmsr"
	:
	: "A"(val), "c"(reg));
}

static inline  qword_t rdmsr(const dword_t reg)
{
    qword_t __return;

    __asm__ __volatile__ (
	"rdmsr"
	: "=A"(__return)
	: "c"(reg)
	);

    return __return;
}

static dword_t escr_addr(dword_t ctr, dword_t num)
{
    return escr_to_addr[ctr][num] == 0 ? 0 : escr_to_addr[ctr][num] + 0x300;
}



/*
 * Function kdebug_get_perfctr (escrmsr, escr, cccr)
 *
 *    Prompt user for a Pentium 4 performance event, including event
 *    mask, counter number, and privilege level.  Results are not
 *    written to MSRs, but are returned via output arguments.
 *    Function returns selected counter number, or -1 upon failure.
 *
 */
PUBLIC static dword_t 
cpu_t::get_perfctr(dword_t *escrmsr, qword_t *escr, qword_t *cccr,
		   dword_t event, dword_t ctr, dword_t replay_tag)
{

    dword_t tbl_max = sizeof(perf_counters)/sizeof(struct perfctr_t);
    dword_t  i, j, k, msr, c_low = 0, c_high = 17;
    char c, t;

#if 0
    printf(__FILE__" line: %d %08x, %08x, %08x, %08x, %08x, %08x\n",
	   __LINE__,escrmsr, escr, cccr, event  ,ctr, replay_tag);
#endif
    if ((event > tbl_max) || (event < 0))
      {
	return ~0;
      }

    *escr = (1 << 4);
    *cccr = (1 << 12) + (3 << 16);

    if ( event < tbl_max )
    {
	/* Predefined event */
	struct perfctr_t *ev = perf_counters + event;
	char defmask_str[17];
	dword_t defmask = 0, mask = 0;

	*escr |= ev->event_select << 25;
	*cccr |= ev->escr_select << 13;
	c_low = ev->ctr_low;
	c_high = ev->ctr_high;

	/* Find default mask. */
	for ( j = 0; ev->mask[j].desc; j++ )
	{
	    defmask_str[j] = ev->mask[j].bitnum < 10 ?
		ev->mask[j].bitnum + '0' : ev->mask[j].bitnum-10 + 'a';
	    defmask |= (1 << ev->mask[j].bitnum);
	}
	defmask_str[j] = 0;

	/* Special handling. */
	switch ( ev->special )
	{
	case 1:
	    /* Edge triggered event. */
	    *cccr |= (1 << 24);
	    break;

	case 2:
	    /* Special handling for bus operations. */
	    *cccr |= (1 << 24);
#if 0
	    c= kdebug_get_choice("Request type",
				 "Read/read Invalidate/Write/writeBack", 'r');
	    switch (c)
	    {
	    case 'r': mask = 0; break;
	    case 'i': mask = 1; break;
	    case 'w': mask = 2; break;
	    case 'b': mask = 3; break;
	    }
	    c= kdebug_get_choice("Requested chunks", "0/1/8", '0');
	    switch (c)
	    {
	    case '0': mask |= (0 << 2); break;
	    case '1': mask |= (1 << 2); break;
	    case '8': mask |= (3 << 2); break;
	    }
	    c = kdebug_get_choice("Request is a demand", "Yes/No", 'y');
	    if ( c == 'y' ) mask |= (1 << 9);
	    c = kdebug_get_choice("Memory type", "Uncachable/writeCombined/"
				  "writeThrough/writeP/writeBack", 'b');
	    switch (c)
	    {
	    case 'u': mask |= (0 << 11); break;
	    case 'c': mask |= (1 << 11); break;
	    case 't': mask |= (4 << 11); break;
	    case 'p': mask |= (5 << 11); break;
	    case 'b': mask |= (6 << 11); break;
	    }
#endif
	    break;

	case 5:
#if 1
	  k = replay_tag;
#else
	    /* Replay events. */	
	    for ( i = 0; i < sizeof(replay_tags)/sizeof(replay_tags[0]); i++ )
		printf("%3d - %s\n", i, replay_tags[i].desc);
	    do {
		printf("Select replay event [0]: ");
		k = kdebug_get_dec();
	    } while ( k >= sizeof(replay_tags)/sizeof(replay_tags[0]) );
#endif
	    wrmsr(IA32_PEBS_ENABLE, replay_tags[k].pebs_enable + (3<<24));
	    wrmsr(IA32_PEBS_MATRIX_VERT, replay_tags[k].pebs_matrix);
	    break;
	}

#if 0
	/* Get event mask. */
	if ( j > 1 )
	{
	    printf("Valid event masks:\n");
	    for ( j = 0; ev->mask[j].desc; j++ )
		printf("  %c - %s\n",
		       ev->mask[j].bitnum < 10 ?
		       ev->mask[j].bitnum + '0' : ev->mask[j].bitnum-10 + 'a',
		       ev->mask[j].desc);
	    printf("Event mask [%s]: ", defmask_str);
	    while ( (t = c = getc()) != '\r' )
	    {
		switch (c)
		{
		case '0' ... '9': c -= '0'; break;
		case 'a' ... 'f': c -= 'a' - 'A';
		case 'A' ... 'F': c = c - 'A' + 10; break;
		default: continue;
		}
		putc(t);
		mask |= (1 << c);
	    }
	    if ( mask == 0 )
		printf("%s", defmask_str);
	    putc('\n');
	}
#endif
	if ( mask == 0 )
	    mask = defmask;

	*escr |= (mask & 0xffff) << 9;
    }
    else
    {
#if 0
	/* No predefined event, set manually. */
	printf("Event select [0]: ");
	*escr |= (kdebug_get_hex() & 0x3f) << 25;
	printf("ESCR select [0]: ");
	*cccr |= (kdebug_get_hex() & 0x07) << 13;
	printf("Event mask [0]: ");
	*escr |= (kdebug_get_hex() & 0xffff) << 9;
#else
	return ~0;
#endif
    }

    /* Get counter number. */
    for (;;)
    {
      //	printf("Counter (%d-%d) [%d]: ", c_low, c_high, c_low);

	msr = escr_addr(ctr, (*cccr >> 13) & 0x7);

	if ( ctr >= c_low && ctr <= c_high )
	{
	    /* Check if ESCR is already in use. */
	    for ( i = 0; i < 18; i++ )
	    {
    		if ( escr_msr[i] == 0 || ctr == i )
		    continue;
		if ( rdmsr(IA32_CCCR_BASE + i) & (1 << 12) )
		{
		    if ( escr_msr[i] == msr )
			break;
		}
		else
		    escr_msr[i] = 0;
	    }

	    if ( i < 18 )
	    {
#if 0
		c = kdebug_get_choice("ESCR already in use",
				      "Overwrite/Try other counter/Cancel",
				      't');
		if ( c == 'o' )  break;
		else if ( c == 'c' ) return ~0;
#else
		return ~0;
#endif
	    }
	    else
		/* ESCR is currently not in use. */
		break;
	}
	else
	  {
	    return ~0;
	  }
	
    }

    /* Should only occur if ESCR and event mask is selected manually. */
    if ( msr == 0 )
    {
	printf("Invalid perf counter/ESCR selector.\n");
	return ~0;
    }
    *escrmsr = escr_msr[ctr] = msr;

    /* Select privilege level where counter should be active. */
#if 1
    c = 'b';
#else
    c = kdebug_get_choice("Privilege level", "User/Kernel/Both", 'b');
#endif
    switch (c)
    {
    case 'u': *escr |= (1 << 2); break;
    case 'b': *escr |= (1 << 2);
    case 'k': *escr |= (1 << 3); break;
    }

    return ctr;
}
