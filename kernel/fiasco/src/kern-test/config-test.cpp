INTERFACE:

EXTENSION class config 
{
public:
  // test task definitions
  static const unsigned test_taskno = 6;
  static const l4_threadid_t test_id0;
  static const unsigned test_prio = 0x10;
  static const unsigned test_mcp = 255;
};

IMPLEMENTATION[test]:

const l4_threadid_t config::test_id0
  = ((l4_threadid_t){id:{0, 0, test_taskno, 0, 0, boot_taskno, 0}});

