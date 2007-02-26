/* -*- c++ -*- */
#ifndef L4_IOSTREAM_H__
#define L4_IOSTREAM_H__

namespace L4 {

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

    IOBackend() : int_mode(10) {}
    virtual void write( char const *str, unsigned len ) = 0;

  private:
    void write( IOModifier m );
    void write( long long int c, int len );
    void write( long long unsigned c, int len );

    Mode get_mode() const { return int_mode; }
    void restore_mode( Mode m ) { int_mode = m; }
    
    int int_mode;
  };

  class BasicOStream
  {
  public:
    BasicOStream( IOBackend *b ) : iob(b) {}
    void write( char const *str, unsigned len ) { if(iob) iob->write(str,len); }
    void write( long long int c, int len ) { if(iob) iob->write(c,len); }
    void write( long long unsigned c, int len ) { if(iob) iob->write(c,len); }
    void write( IOModifier m ) { if(iob) iob->write(m); }

    IOBackend::Mode get_be_mode() const { if(iob) return iob->get_mode(); return 0; }
    void restore_be_mode(IOBackend::Mode m) { if(iob) iob->restore_mode(m); }

  private:
    IOBackend *iob;
  
  };
 
  IOModifier const hex(16);
  IOModifier const dec(10);

  extern BasicOStream cout;
  extern BasicOStream cerr;

};

inline L4::BasicOStream &operator << ( L4::BasicOStream &s, char const *str )
{
  unsigned l=0;
  for(; str[l]!=0; l++);
  s.write(str,l);
  return s;
}

inline L4::BasicOStream &operator << ( L4::BasicOStream &s, unsigned u )
{
  s.write((long long unsigned)u,-1);
  return s;
}

inline L4::BasicOStream &operator << ( L4::BasicOStream &s, void *u )
{
  long unsigned x = (long unsigned)u;
  s.write((long long unsigned)x,-1);
  return s;
}

inline L4::BasicOStream &operator << ( L4::BasicOStream &s, L4::IOModifier m )
{
  s.write(m);
  return s;
}

#endif /* L4_IOSTREAM_H__ */
