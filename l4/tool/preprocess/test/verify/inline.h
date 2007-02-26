// AUTOMATICALLY GENERATED -- DO NOT EDIT!         -*- c++ -*-

#ifndef inline_h
#define inline_h

//
// INTERFACE definition follows 
//

#line 2 "inline.cpp"

class Foo
{

public:  
#line 39 "inline.cpp"
  // This inline funtion is public only because it is needed by an
  // extern-"C" function.  So we do not want to export it.
  
  void 
  bar();
  
#line 48 "inline.cpp"
  // Try both NOEXPORT and NEEDED.
  
  void 
  baz();

private:  
#line 13 "inline.cpp"
  // Test dependency-chain resolver
  
  
  bool 
  private_func();
};
#line 6 "inline.cpp"

class Bar
{

public:  
#line 34 "inline.cpp"
  void
  public_func();

private:  
#line 22 "inline.cpp"
  bool 
  private_func();
  
#line 28 "inline.cpp"
  void 
  another_private_func();
};

#line 55 "inline.cpp"
extern "C" 
void function(Foo* f);

//
// IMPLEMENTATION of inline functions (and needed classes)
//


#line 12 "inline.cpp"

// Test dependency-chain resolver


inline bool 
Foo::private_func()
{
}

#line 20 "inline.cpp"


inline bool 
Bar::private_func()
{
}

#line 26 "inline.cpp"


inline void 
Bar::another_private_func()
{
}

#line 32 "inline.cpp"


inline void
Bar::public_func()
{
}

#endif // inline_h
