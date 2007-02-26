/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx_log_io package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/cxx/iostream.h>
#include <l4/sys/kdebug.h>

extern "C" char LOG_tag[9];
extern "C" void LOG_server_outstring( char const *buffer );

namespace L4 {

  class LogIOBackend : public IOBackend
  {
  public:
    LogIOBackend();
  protected:
    void write(char const *str, unsigned len);
  private:
    char obuf[82];
    int  pos;

    unsigned add_to_obuf( char const *&str, unsigned &len );
  };

  LogIOBackend::LogIOBackend()
    : pos(sizeof(LOG_tag) + 1)
  { 
    for( unsigned x=0;x<sizeof(LOG_tag)-1;x++ )
      obuf[x] = LOG_tag[x]?LOG_tag[x]:' ';

    obuf[sizeof(LOG_tag)-1] = '|';
    obuf[sizeof(LOG_tag)  ] = ' ';
    obuf[sizeof(obuf)-2   ] = '\n'; 
  }

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
        obuf[nl+pos] = '\n'; 
	nl++;
	ret = 2;
      }
    
    pos += nl;
#if 0
    if ( obuf[pos-1] == '\n' || nl == 0 )
      ret = 1;
#endif
    return ret;
  }
  
  void LogIOBackend::write(char const *str, unsigned len)
  {
    while(len) {
      unsigned r;
      if((r=add_to_obuf(str,len))) {
        obuf[pos] = 0;
	//outnstring(obuf,pos);
        LOG_server_outstring(obuf);
	if (r==1)
	  obuf[sizeof(LOG_tag)-1] ='|';
	else
	  obuf[sizeof(LOG_tag)-1] =':';

        pos = sizeof(LOG_tag) + 1;
      }
    }  
  }

  namespace {
    LogIOBackend iob;
  };
  
  BasicOStream cout(&iob);
  BasicOStream cerr(&iob);
};
