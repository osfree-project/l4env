/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx_linux_io package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/cxx/iostream.h>

extern "C" int write( int fd, void *buf, unsigned count );
namespace L4 {

  class LogIOBackend : public IOBackend
  {
  public:
    LogIOBackend(int fd);
  protected:
    void write(char const *str, unsigned len);
  private:
    char obuf[82];
    int  pos;
    int  fd;

    unsigned add_to_obuf( char const *&str, unsigned &len );
  };

  LogIOBackend::LogIOBackend(int fd)
    : pos(0), fd(fd)
  {}

  unsigned LogIOBackend::add_to_obuf( char const *&str, unsigned &len )
  {
    unsigned nl = 0;
    unsigned ret = 0;
    for (;nl<(sizeof(obuf)-pos-1) && (nl<len) && (str[nl] != '\n');nl++)
      obuf[nl+pos] = str[nl];

    str += nl;
    len -= nl;
    if (nl<len && str[nl] == '\n')
      {
	str ++;
	len --;
        obuf[nl+pos] = '\n'; 
	pos ++;
	ret = 1;
      }

    if (nl == (sizeof(obuf)-pos-1))
      {
	nl++;
	ret = 1;
      }
    
    pos += nl;
    return ret;
  }
  
  void LogIOBackend::write(char const *str, unsigned len)
  {
    while(len) {
      unsigned r;
      if((r=add_to_obuf(str,len))) {
        obuf[pos] = 0;
	::write(fd,obuf,pos);
        pos = 0;
      }
    }  
  }

  namespace {
    LogIOBackend cout_iob(1);
    LogIOBackend cerr_iob(2);
  };
  
  BasicOStream cout(&cout_iob);
  BasicOStream cerr(&cerr_iob);
};
