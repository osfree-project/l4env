/* -*- c++ -*- */
#ifndef ARM_L4_TEST_H__
#define ARM_L4_TEST_H__

class Test
{
public:
  enum {
    max_tests = 32,
  };

  Test( char const *name ) : _name(name) 
  { add_test(this); }

  char const *const name() const { return _name; }

  virtual bool run() = 0;
  
private:
  char const *const _name;

public:
  static void add_test( Test *t );
  static void run_tests();
  
private:
  static Test *tests[max_tests];


};


#endif /* ARM_L4_TEST_H__ */
