/* -*- c++ -*- */
/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_IOSTREAM_H__
#define L4_IOSTREAM_H__

namespace L4 
{

  class IOModifier
  {
  public:
    IOModifier( int x ) : mod(x) {}
    bool operator == (IOModifier o) { return mod == o.mod; }
    bool operator != (IOModifier o) { return mod != o.mod; }
    int mod;
  };

  class IOBackend
  {
  public:
    typedef int Mode;

  protected:
    friend class BasicOStream;

    IOBackend() 
      : int_mode(10) 
    {}

    virtual ~IOBackend() {}
    
    virtual void write(char const *str, unsigned len) = 0;

  private:
    void write(IOModifier m);
    void write(long long int c, int len);
    void write(long long unsigned c, int len);
    void write(long long unsigned c, unsigned char base = 10, 
	       unsigned char len = 0, char pad = ' ');

    Mode mode() const 
    { return int_mode; }
    
    void mode(Mode m) 
    { int_mode = m; }
    
    int int_mode;
  };

  class BasicOStream
  {
  public:
    BasicOStream(IOBackend *b) 
      : iob(b) 
    {}
    
    void write(char const *str, unsigned len) 
    { if(iob) iob->write(str,len); }
    
    void write(long long int c, int len) 
    { if(iob) iob->write(c,len); }
    
    void write(long long unsigned c, unsigned char base = 10,
	       unsigned char len = 0, char pad = ' ')
    { if(iob) iob->write(c, base, len, pad); }
    
    void write(long long unsigned c, int len)
    { if(iob) iob->write(c,len); }
    
    void write(IOModifier m) 
    { if(iob) iob->write(m); }

    IOBackend::Mode be_mode() const 
    { if(iob) return iob->mode(); return 0; }
    
    void be_mode(IOBackend::Mode m) 
    { if(iob) iob->mode(m); }

  private:
    IOBackend *iob;
  
  };

  class IONumFmt
  {
  public:
    IONumFmt(unsigned long long n, unsigned char base = 10, 
	     unsigned char len = 0, char pad = ' ')
      : n(n), base(base), len(len), pad(pad)
    {}

  BasicOStream &print(BasicOStream &o) const;

  private:
    unsigned long long n;
    unsigned char base, len;
    char pad;
  };
 
  extern IOModifier const hex;
  extern IOModifier const dec;

  extern BasicOStream cout;
  extern BasicOStream cerr;


  inline
  BasicOStream &IONumFmt::print(BasicOStream &o) const
  {
    o.write(n, base, len, pad);
    return o;
  }

  
};

inline 
L4::BasicOStream &
operator << (L4::BasicOStream &s, char const * const str)
{
  if (!str)
    {
      s.write("(NULL)", 6);
      return s;
    }
  
  unsigned l=0;
  for(; str[l]!=0; l++);
  s.write(str,l);
  return s;
}

inline
L4::BasicOStream &
operator << (L4::BasicOStream &s, signed short u)
{
  s.write((long long signed)u,-1);
  return s;
}

inline
L4::BasicOStream &
operator << (L4::BasicOStream &s, signed u)
{
  s.write((long long signed)u,-1);
  return s;
}

inline 
L4::BasicOStream &
operator << (L4::BasicOStream &s, signed long u)
{
  s.write((long long signed)u,-1);
  return s;
}

inline
L4::BasicOStream &
operator << (L4::BasicOStream &s, signed long long u)
{
  s.write(u,-1);
  return s;
}

inline
L4::BasicOStream &
operator << (L4::BasicOStream &s, unsigned short u)
{
  s.write((long long unsigned)u,-1);
  return s;
}

inline
L4::BasicOStream &
operator << (L4::BasicOStream &s, unsigned u)
{
  s.write((long long unsigned)u,-1);
  return s;
}

inline 
L4::BasicOStream &
operator << (L4::BasicOStream &s, unsigned long u)
{
  s.write((long long unsigned)u,-1);
  return s;
}

inline
L4::BasicOStream &
operator << (L4::BasicOStream &s, unsigned long long u)
{
  s.write(u,-1);
  return s;
}

inline 
L4::BasicOStream &
operator << (L4::BasicOStream &s, void const *u)
{
  long unsigned x = (long unsigned)u;
  L4::IOBackend::Mode mode = s.be_mode();
  s.write(L4::hex);
  s.write((long long unsigned)x,-1);
  s.be_mode(mode);
  return s;
}

inline 
L4::BasicOStream &
operator << (L4::BasicOStream &s, L4::IOModifier m)
{
  s.write(m);
  return s;
}

inline
L4::BasicOStream &
operator << (L4::BasicOStream &s, char c)
{
  s.write( &c, 1 );
  return s;
}
  
inline
L4::BasicOStream &
operator << (L4::BasicOStream &o, L4::IONumFmt const &n)
{ return n.print(o); }

#endif /* L4_IOSTREAM_H__ */
