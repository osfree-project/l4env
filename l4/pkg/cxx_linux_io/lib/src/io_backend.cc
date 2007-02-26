/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx_linux_io package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <cstdio>
#include <l4/cxx/iostream.h>

namespace L4 {

  class LogIOBackend : public IOBackend
  {
  public:
    LogIOBackend(FILE *stream);
  protected:
    void write(char const *str, unsigned len);
  private:
    FILE *stream;
  };

  LogIOBackend::LogIOBackend(FILE *stream)
    : stream(stream)
  {}

  void LogIOBackend::write(char const *str, unsigned len)
  {
    size_t pos = 0;
    while(len) 
      {
	size_t l = ::fwrite(str + pos, 1, len, stream);
        pos += l;
	len -= l;
      }
  }

  namespace {
    LogIOBackend cout_iob(stdout);
    LogIOBackend cerr_iob(stderr);
  };
  
  BasicOStream cout(&cout_iob);
  BasicOStream cerr(&cerr_iob);
};
