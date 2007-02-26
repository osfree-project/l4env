#include <iostream>
#include <string>
#include <vector>

typedef std::vector<std::string> str_vect;

str_vect strings;

int main (void)
{
  strings.push_back("Foo 1");
  strings.push_back("Bar 1");
  strings.push_back("Baz 1");
  strings.push_back("Joe 1");
  strings.push_back("Lizi 1");

  for (str_vect::const_iterator i = strings.begin(); i != strings.end(); ++i)
    std::cout << (*i) << "\n";

  return 0;
}
