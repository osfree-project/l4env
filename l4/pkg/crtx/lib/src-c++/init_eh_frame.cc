class Start_stop
{
public:
  Start_stop();
  ~Start_stop();
};

extern "C" char __eh_frame_start__[];
extern "C" void __register_frame (const void *begin);
extern "C" void __deregister_frame (const void *begin);


Start_stop::Start_stop()
{
  __register_frame(__eh_frame_start__);
}

Start_stop::~Start_stop()
{
  __deregister_frame(__eh_frame_start__);
}

static Start_stop __start__stop__ __attribute__((init_priority(101)));

