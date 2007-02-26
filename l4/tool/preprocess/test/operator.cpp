INTERFACE:

class Foo
{
};

IMPLEMENTATION:

PUBLIC 
void * 
Foo::operator new (size_t)	// funny comment
{
}

PUBLIC 
Foo&
Foo::operator + (const Foo&)	// funny comment
{
}

PUBLIC 
Foo&
Foo::operator = (const Foo&)	// funny comment
{
}

PUBLIC 
Foo&
Foo::operator * (const Foo&)	// funny comment
{
}

template <typename T, typename A>
std::vector<T, A >& 
operator << (std::vector<T, A>& in, const T& new_elem)
{
  in.push_back (new_elem);
  return in;
}
