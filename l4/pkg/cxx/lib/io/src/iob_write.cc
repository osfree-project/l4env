/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/cxx/iostream.h>

namespace L4 
{

  IOModifier const hex(16);
  IOModifier const dec(10);

  static char const hex_chars[] = "0123456789abcdef";

  void IOBackend::write(IOModifier m)
  {
    if(m == dec)
      int_mode = 10;
    else if(m == hex)
      int_mode = 16;
  }

  void IOBackend::write(long long int u, int len)
  {
    char buffer[20];
    int  pos = 20;
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
  
  void IOBackend::write(long long unsigned u, int len)
  {
    char buffer[20];
    int  pos = 20;
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

  void IOBackend::write(long long unsigned u, unsigned char base,
                        unsigned char len, char pad)
  {
    char buffer[30];
    unsigned pos = sizeof(buffer);
    buffer[sizeof(buffer)-1] = '0';
    while (pos > 0 && u)
      {
        buffer[--pos] = hex_chars[(u % base)];
        u /= base;
      }

    if (len >sizeof(buffer))
      len = sizeof(buffer);

    if (len && sizeof(buffer)-pos > len)
      pos = sizeof(buffer) - len;
    
    while (sizeof(buffer)-pos < len)
      buffer[--pos] = pad;
    
    if (pos==sizeof(buffer))
      pos--;

    write(buffer + pos, sizeof(buffer)-pos);
  }
};
