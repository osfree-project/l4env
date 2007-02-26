// AUTOMATICALLY GENERATED -- DO NOT EDIT!         -*- c++ -*-

#include "parser.h"
#include "parser_i.h"


#line 28 "parser.cpp"

// Try function arguments
unsigned
somefunc(unsigned (*func1)(), 
	 unsigned (*func2)())
{
}
#line 35 "parser.cpp"

// Try to initialize a nested structure object.
struct Foo::Bar some_bar = { 1 };

#line 38 "parser.cpp"

// And add a Foo function
void Foo::func()
{}

#line 42 "parser.cpp"

// Try default arguments
void Foo::bar(int i, int j)
{}

#line 46 "parser.cpp"

// Try a constructor with weird syntax

Foo::Foo()
  : something (reinterpret_cast<Bar*>(Baz::bla()))
{}

#line 52 "parser.cpp"

// Try implementing an already-declared function
int 
Foo::alreadythere()
{}
#line 74 "parser.cpp"

void find_this ();
#line 80 "parser.cpp"

#ifdef HEILIGE_WEIHNACHT
#line 82 "parser.cpp"
static void present_this ();
#line 83 "parser.cpp"
#endif
