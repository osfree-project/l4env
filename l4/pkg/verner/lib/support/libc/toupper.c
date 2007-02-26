#include "cr7libc.h"

inline int cr7_toupper(int ch) {
  if ( (unsigned int)(ch - 'a') < 26u )
    ch += 'A' - 'a';
  return ch;
}

