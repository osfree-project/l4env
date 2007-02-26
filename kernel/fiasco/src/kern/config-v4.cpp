INTERFACE:

EXTENSION class Config
{
public:
  static const L4_uid kernel_id;

  enum {
    ABI_V4 = 1,
    
    // Define UTCB and UTCB_AREA dimensions.  These values are distributed
    // in the KIP.
    MIN_UTCB_AREA_SIZE	= 9,	// log2
    UTCB_ALIGNMENT	= 9,	// log2
    UTCB_SHIFT		= 9,

    // derived values
    UTCB_SIZE		= 1 << UTCB_SHIFT,
    UTCB_SIZE_MULTIPLIER= 1 << (UTCB_SHIFT - UTCB_ALIGNMENT),

    // Define the counts of threads the sigma0/root tasks may have.
    MAX_SIGMA0_THREADS	= 8,	// log2
    MAX_ROOT_THREADS	= 8,	// log2

    // Set where to map the UTCBs for the sigma0 and the root task.
    SIGMA0_UTCB_AREA_BASE	= 0x30000000, // Assumption: page aligned
    ROOT_UTCB_AREA_BASE		= 0x30000000, // --"--

    // Calculate the sizes of UTCB areas.
    SIGMA0_UTCB_AREA_SIZE	= UTCB_SHIFT + MAX_SIGMA0_THREADS,
    ROOT_UTCB_AREA_SIZE		= UTCB_SHIFT + MAX_ROOT_THREADS,

    // Set the UTCB locations (i.e. local IDs) for the root servers.
    // If you change these values, keep in mind that
    //  - the UTCB has to fit into the corresponding UTCB_AREA defined above.
    //  - the location must be educible as a local thread ID, that means
    //    the lowermost 6 bits have to be 0.
    SIGMA0_UTCB_LOCATION = SIGMA0_UTCB_AREA_BASE + Utcb::UTCB_PTR_OFFSET,
    ROOT_UTCB_LOCATION	 = ROOT_UTCB_AREA_BASE   + Utcb::UTCB_PTR_OFFSET,

    // Set where to map the KIP for the sigma0 and the root task.
    SIGMA0_KIP_LOCATION = 0x31000000,
    ROOT_KIP_LOCATION = 0x31000000,

    KERNEL_ID = 0x100,
  };
};


IMPLEMENTATION[v4]:

// define some constants which need a memory representation
const L4_uid Config::kernel_id (KERNEL_ID, 1);  // gthread, version
