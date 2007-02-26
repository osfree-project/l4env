#include <l4/cxx/iostream.h>

namespace L4 {

  static char hex_chars[] = "0123456789abcdef";

  void IOBackend::write( IOModifier m )
  {
    if(m == dec)
      int_mode = 10;
    else if(m == hex)
      int_mode = 16;
  }

  void IOBackend::write( long long int u, int len )
  {
    char buffer[20];
    char pos = 20;
    bool sign = u < 0;
    if(sign) u=-u;
    buffer[19] = '0';
    while(u)
      {
        buffer[--pos] = hex_chars[(u % int_mode)];
        u /= int_mode;
      }
    if(pos==20)
      pos = 19;
    if(sign)
      buffer[--pos] = '-';

    write(buffer + pos, 20-pos);
  }
  
  void IOBackend::write( long long unsigned u, int len )
  {
    char buffer[20];
    char pos = 20;
    buffer[19] = '0';
    while(u)
      {
        buffer[--pos] = hex_chars[(u % int_mode)];
        u /= int_mode;
      }
    if(pos==20)
      pos = 19;

    write(buffer + pos, 20-pos);
  }
};
