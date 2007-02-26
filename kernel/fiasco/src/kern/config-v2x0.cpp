INTERFACE:

EXTENSION class Config
{
public:

  static const L4_uid kernel_id;
  static const unsigned kernel_taskno = 0;

  static const L4_uid sigma0_id; 
  static const unsigned sigma0_taskno = 2;

  static const L4_uid boot_id;
  static const unsigned boot_taskno = 4;

  enum { ABI_V4 = 0 };
};


IMPLEMENTATION[v2x0]:

// define some constants which need a memory representation
const L4_uid Config::kernel_id( L4_uid::NIL );
const L4_uid Config::sigma0_id( sigma0_taskno, 0, 0, boot_taskno );
const L4_uid Config::boot_id  ( boot_taskno, 0, 0, kernel_taskno );
