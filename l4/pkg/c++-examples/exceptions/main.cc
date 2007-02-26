#include <cstdio>
#include <cstdlib>

#define CHECK(e) \
{ if (e) { puts("OK"); } else { printf("FAILED [%s]\n", #e); } }

class Gax
{
public:
  Gax(int val) : value(val) {}
  virtual int val();

private:
  int value;
};

int Gax::val() { return value; }

class Inner
{
public:
  Inner();
  void fun();
  ~Inner();

  static int state;
};

class Outer
{
public:
  Outer();
  void fun();
  ~Outer();

  static int state;
};

int Inner::state = 0;
int Outer::state = 0;
  
Inner::Inner()
{
  puts("Construct Inner");
  state = 1;
}

Inner::~Inner()
{
  puts("Destroy Inner");
  state = 2;
}

void Inner::fun()
{
  puts("Inner::fun()");
  throw new Gax(12);
  puts("  Never reached");
}

Outer::Outer()
{
  puts("Construct Outer");
  state = 1;
}

Outer::~Outer()
{
  puts("Destroy Outer");
  state = 2;
}

void Outer::fun()
{
  Inner in;
  puts("Outer::fun(): calls Inner::fun()");
  in.fun();
  puts("Outer::fun(): came back");
}

int main (void)
{
  printf("Do It\n");
  try {
    printf("Run Gix cons\n");
    Outer out;
    out.fun();
    printf("FAILED: Exception not thrown\n");
  } 
#if 1
  catch (int g) 
  {
    printf("Exception: int=%d\n", g);
    exit(0);
  }
#endif
  catch (Gax *g)
  {
    printf("Exception: Gax=%d\n", g->val());
    delete g;

    // assert that the destructors where called
    CHECK (Inner::state == 2 && Outer::state == 2);
    
  }
  catch (char const *g)
  {
    printf("Strexc: %s\n", g);
  }
  catch (...) 
  {
    printf("Some unknown execption\n");
  }
  return 0;
}
