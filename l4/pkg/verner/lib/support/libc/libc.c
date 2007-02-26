#include "cr7libc.h"
#include "dietstdio.h"
#include "dietfeatures.h"

#include <stdlib.h> //strto(u)l
#include <string.h>
//#include <oskit/c/string.h> //strchr , memmove 

//#include <oskit/console.h>


#ifndef _LIMITS_H
#define _LIMITS_H
#if __WORDSIZE == 64
#define LONG_MAX 9223372036854775807L
#define ULONG_MAX 18446744073709551615UL
#else
#define LONG_MAX 2147483647L
#define ULONG_MAX 4294967295UL
#endif
#define LONG_MIN (-LONG_MAX - 1L)
#endif
/* internal prototypes */
int isalpha ( int ch );
int isdigit ( int ch );
int isspace ( int ch );
int isalnum ( int ch );
int isxdigit( int ch );


int isspace ( int ch )
{
    return (unsigned int)(ch - 9) < 5u  ||  ch == ' ';
}

int isalpha ( int ch ) {
    return (unsigned int)((ch | 0x20) - 'a') < 26u;
}
int isdigit ( int ch ) {
    return (unsigned int)(ch - '0') < 10u;
}

int isalnum ( int ch ) {
    return (unsigned int)((ch | 0x20) - 'a') < 26u  ||
           (unsigned int)( ch         - '0') < 10u;
}

int isxdigit( int ch )
{
    return (unsigned int)( ch         - '0') < 10u  || 
           (unsigned int)((ch | 0x20) - 'a') <  6u;
}



#if 0
#define ABS_LONG_MIN 2147483648UL
long int strtol(const char *nptr, char **endptr, int base)
{
  int neg=0;
  unsigned long int v;
  const char*orig=nptr;

  while(isspace(*nptr)) nptr++;

  if (*nptr == '-' && isalnum(nptr[1])) { neg=-1; ++nptr; }
  v=strtoul(nptr,endptr,base);
  if (endptr && *endptr==nptr) *endptr=(char *)orig;
  if (v>=ABS_LONG_MIN) {
    if (v==ABS_LONG_MIN && neg) {
      errno=0;
      return v;
    }
    errno=ERANGE;
    return (neg?LONG_MIN:LONG_MAX);
  }
  return (neg?-v:v);
}

unsigned long int strtoul(const char *ptr, char **endptr, int base)
{
  int neg = 0, overflow = 0;
  unsigned long int v=0;
  const char* orig;
  const char* nptr=ptr;

  while(isspace(*nptr)) ++nptr;

  if (*nptr == '-') { neg=1; nptr++; }
  else if (*nptr == '+') ++nptr;
  orig=nptr;
  if (base==16 && nptr[0]=='0') goto skip0x;
  if (base) {
    register unsigned int b=base-2;
    if (__unlikely(b>34)) { errno=EINVAL; return 0; }
  } else {
    if (*nptr=='0') {
      base=8;
skip0x:
      if ((nptr[1]=='x'||nptr[1]=='X') && isxdigit(nptr[2])) {
	nptr+=2;
	base=16;
      }
    } else
      base=10;
  }
  while(__likely(*nptr)) {
    register unsigned char c=*nptr;
    c=(c>='a'?c-'a'+10:c>='A'?c-'A'+10:c<='9'?c-'0':0xff);
    if (__unlikely(c>=base)) break;	/* out of base */
    {
      register unsigned long x=(v&0xff)*base+c;
      register unsigned long w=(v>>8)*base+(x>>8);
      if (w>(ULONG_MAX>>8)) overflow=1;
      v=(w<<8)+(x&0xff);
    }
    ++nptr;
  }
  if (__unlikely(nptr==orig)) {		/* no conversion done */
//err_conv:
    nptr=ptr;
    errno=EINVAL;
    v=0;
  }
  if (endptr) *endptr=(char *)nptr;
  if (overflow) {
    //errno=ERANGE;
    return ULONG_MAX;
  }
  return (neg?-v:v);
}
#endif

double cr7_strtod(const char *nptr, char **endptr)
{
    return strtod(nptr,endptr);
};
long int cr7_strtol(const char *nptr, char **endptr, int base)
{
    return strtol(nptr,endptr,base);
};
unsigned long int cr7_strtoul(const char *nptr, char **endptr, int base)
{
    return strtoul(nptr,endptr,base);
};
       


#define A_WRITE(fn,buf,sz)	((fn)->put((void*)(buf),(sz),(fn)->data))

//Vars
struct str_data {
  unsigned char* str;
  size_t len;
  size_t size;
};

//interne Prototypen

int cr7_vprintf(const char *format, va_list ap);
int cr7_vsnprintf(char* str, size_t size, const char *format, va_list arg_ptr);
int cr7_vsprintf(char *dest,const char *format, va_list arg_ptr);

int __stdio_outs(const char *s,size_t len) __attribute__((weak));
#ifdef USE_OSKIT
int __lltostr(char *s, int size, unsigned long long i, int base, char UpCase);
#endif
int __dtostr(double d,char *buf,unsigned int maxlen,unsigned int prec,unsigned int prec2);
int __ltostr(char *s, unsigned int size, unsigned long i, unsigned int base, int UpCase);
int __v_printf(struct arg_printf* fn, const unsigned char *format, va_list arg_ptr);


//memmove strtoul strcgr strtol




/* convert double to string.  Helper for sprintf. */
int __dtostr(double d,char *buf,unsigned int maxlen,unsigned int prec,unsigned int prec2) {
#if 1
  unsigned long long *x=(unsigned long long *)&d;
  /* step 1: extract sign, mantissa and exponent */
  signed long e=((*x>>52)&((1<<11)-1))-1023;
#else
#if __BYTE_ORDER == __LITTLE_ENDIAN
  signed long e=(((((unsigned long*)&d)[1])>>20)&((1<<11)-1))-1023;
#else
  signed long e=(((*((unsigned long*)&d))>>20)&((1<<11)-1))-1023;
#endif
#endif
/*  unsigned long long m=*x & ((1ull<<52)-1); */
  /* step 2: exponent is base 2, compute exponent for base 10 */
  signed long e10=1+(long)(e*0.30102999566398119802); /* log10(2) */
  /* step 3: calculate 10^e10 */
  unsigned int i;
  double backup=d;
  double tmp;
  char *oldbuf=buf;

  /* Wir iterieren von Links bis wir bei 0 sind oder maxlen erreicht
   * ist.  Wenn maxlen erreicht ist, machen wir das nochmal in
   * scientific notation.  Wenn dann von prec noch was übrig ist, geben
   * wir einen Dezimalpunkt aus und geben prec2 Nachkommastellen aus.
   * Wenn prec2 Null ist, geben wir so viel Stellen aus, wie von prec
   * noch übrig ist. */
  if (d==0.0) {
    prec2=prec2==0?1:prec2+2;
    prec2=prec2>maxlen?8:prec2;
    for (i=0; i<prec2; ++i) buf[i]='0';
    buf[1]='.'; buf[i]=0;
    return i;
  }

  if (d < 0.0) { d=-d; *buf='-'; --maxlen; buf++; }
  
   /*
      Perform rounding. It needs to be done before we generate any
      digits as the carry could propagate through the whole number.
   */
   
   tmp = 0.5;
   for (i = 0; i < prec2; i++) { tmp *= 0.1; }
   d += tmp;
  
/*  printf("e=%d e10=%d prec=%d\n",e,e10,prec); */
  if (e10>0) {
    int first=1;	/* are we about to write the first digit? */
    tmp = 10.0;
    i=e10;
    while (i>10) { tmp=tmp*1e10; i-=10; }
    while (i>1) { tmp=tmp*10; --i; }
    /* the number is greater than 1. Iterate through digits before the
     * decimal point until we reach the decimal point or maxlen is
     * reached (in which case we switch to scientific notation). */
    while (tmp>0.9) {
      char digit;
      double fraction=d/tmp;
	digit=(int)(fraction);		/* floor() */
      if (!first || digit) {
	first=0;
	*buf=digit+'0'; ++buf;
	if (!maxlen) {
	  /* use scientific notation */
	  int len=__dtostr(backup/tmp,oldbuf,maxlen,prec,prec2);
	  int initial=1;
	  if (len==0) return 0;
	  maxlen-=len; buf+=len;
	  if (maxlen>0) {
	    *buf='e';
	    ++buf;
	  }
	  --maxlen;
	  for (len=1000; len>0; len/=10) {
	    if (e10>=len || !initial) {
	      if (maxlen>0) {
		*buf=(e10/len)+'0';
		++buf;
	      }
	      --maxlen;
	      initial=0;
	      e10=e10%len;
	    }
	  }
	  if (maxlen>0) goto fini;
	  return 0;
	}
	d-=digit*tmp;
	--maxlen;
      }
      tmp/=10.0;
    }
  }
  else
  {
     tmp = 0.1;
  }

  if (buf==oldbuf) {
    if (!maxlen) return 0; --maxlen;
    *buf='0'; ++buf;
  }
  if (prec2 || prec>(unsigned int)(buf-oldbuf)+1) {	/* more digits wanted */
    if (!maxlen) return 0; --maxlen;
    *buf='.'; ++buf;
    prec-=buf-oldbuf-1;
    if (prec2) prec=prec2;
    if (prec>maxlen) return 0;
    while (prec>0) {
      char digit;
      double fraction=d/tmp;
      digit=(int)(fraction);		/* floor() */
      *buf=digit+'0'; ++buf;
      d-=digit*tmp;
      tmp/=10.0;
      --prec;
    }
  }
fini:
  *buf=0;
  return buf-oldbuf;
}

/**************************************************/
/* long long 2 str                                */
/**************************************************/
#ifdef USE_OSKIT
int __lltostr(char *s, int size, unsigned long long i, int base, char UpCase)
{
  char *tmp;
  unsigned int j=0;

  s[--size]=0;

  tmp=s+size;

  if ((base==0)||(base>36)) base=10;

  j=0;
  if (!i)
  {
    *(--tmp)='0';
    j=1;
}

  while((tmp>s)&&(i))
  {
    tmp--;
    if ((*tmp=i%base+'0')>'9') *tmp+=(UpCase?'A':'a')-'9'-1;
    i=i/base;
    j++;
  }
  memmove(s,tmp,j+1);

  return j;
}
#endif

int __ltostr(char *s, unsigned int size, unsigned long i, unsigned int base, int UpCase)
{
  char *tmp;
  unsigned int j=0;

  s[--size]=0;

  tmp=s+size;

  if ((base==0)||(base>36)) base=10;

  j=0;
  if (!i)
  {
    *(--tmp)='0';
    j=1;
  }

  while((tmp>s)&&(i))
  {
    tmp--;
    if ((*tmp=i%base+'0')>'9') *tmp+=(UpCase?'A':'a')-'9'-1;
    i=i/base;
    j++;
  }
  memmove(s,tmp,j+1);

  return j;
}

static inline unsigned int skip_to(const unsigned char *format) {
  int unsigned nr;
  for (nr=0; format[nr] && (format[nr]!='%'); ++nr);
  return nr;
}


static char* pad_line[16]= { "                ", "0000000000000000", };
static inline int write_pad(struct arg_printf* fn, int len, int padwith) {
  int nr=0;
  for (;len>15;len-=16,nr+=16) {
    A_WRITE(fn,pad_line[(padwith=='0')?1:0],16);
  }
  if (len>0) {
    A_WRITE(fn,pad_line[(padwith=='0')?1:0],(unsigned int)len); nr+=len;
  }
  return nr;
}

int __v_printf(struct arg_printf* fn, const unsigned char *format, va_list arg_ptr)
{
  int len=0;

  while (*format) {
    unsigned int sz = skip_to(format);
    if (sz) {
      A_WRITE(fn,format,sz); len+=sz;
      format+=sz;
    }
    if (*format=='%') {
      char buf[128];

      unsigned char ch, *s, padwith=' ';

      char flag_in_sign=0;
      char flag_upcase=0;
      char flag_hash=0;
      char flag_left=0;
      char flag_space=0;
      char flag_sign=0;
      char flag_dot=0;
      signed char flag_long=0;

      unsigned int base;
      unsigned int width=0, preci=0;

      int number=0;
#ifdef WANT_LONGLONG_PRINTF
      long long llnumber=0;
#endif

      ++format;
inn_printf:
      switch(ch=*format++) {
      case 0:
	return -1;
	break;

      /* FLAGS */
      case '#':
	flag_hash=1;
      case 'z':
	goto inn_printf;

      case 'h':
	--flag_long;
	goto inn_printf;
      case 'L':
	++flag_long; /* fall through */
      case 'l':
	++flag_long;
	goto inn_printf;

      case '0':
	padwith='0';
	goto inn_printf;

      case '-':
	flag_left=1;
	goto inn_printf;

      case ' ':
	flag_space=1;
	goto inn_printf;

      case '+':
	flag_sign=1;
	goto inn_printf;

      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
	if(flag_dot) return -1;
	width=strtoul(format-1,(char**)&s,10);
	format=s;
	goto inn_printf;

      case '*':
	width=va_arg(arg_ptr,int);
	goto inn_printf;

      case '.':
	flag_dot=1;
	if (*format=='*') {
	  preci=va_arg(arg_ptr,int);
	  ++format;
	} else {
	  long int tmp=strtol(format,(char**)&s,10);
	  preci=tmp<0?0:tmp;
	  format=s;
	}
	goto inn_printf;

      /* print a char or % */
      case 'c':
	ch=(char)va_arg(arg_ptr,int);
      case '%':
	A_WRITE(fn,&ch,1); ++len;
	break;

      /* print a string */
      case 's':
	s=va_arg(arg_ptr,char *);
#ifdef WANT_NULL_PRINTF
	if (!s) s="(null)";
#endif
	sz = strlen(s);
	if (flag_dot && sz>preci) sz=preci;

print_out:
	if (width && (!flag_left)) {
	  if (flag_in_sign) {
	    A_WRITE(fn,s,1); ++len;
	    ++s; --sz;
	    --width;
	  }
//	  len+=write_pad(fn,(signed int)width-(signed int)sz,padwith);
	  if (flag_dot) {
	    len+=write_pad(fn,(signed int)width-(signed int)preci,padwith);
	    len+=write_pad(fn,(signed int)preci-(signed int)sz,'0');
	  } else
	    len+=write_pad(fn,(signed int)width-(signed int)sz,padwith);
	}
	A_WRITE(fn,s,sz); len+=sz;
	if (width && (flag_left)) {
	  len+=write_pad(fn,(signed int)width-(signed int)sz,' ');
	}
	break;

      /* print an integer value */
      case 'b':
	base=2;
	sz=0;
	goto num_printf;
      case 'p':
	flag_hash=1;
	ch='x';
      case 'X':
	flag_upcase=(ch=='X');
      case 'x':
	base=16;
	sz=0;
	if (flag_hash) {
	  buf[1]='0';
	  buf[2]=ch;
	  sz=2;
	}
	goto num_printf;
      case 'd':
      case 'i':
	flag_in_sign=1;
      case 'u':
	base=10;
	sz=0;
	goto num_printf;
      case 'o':
	base=8;
	sz=0;
	if (flag_hash) {
	  buf[1]='0';
	  ++sz;
	}

num_printf:
	s=buf+1;

	if (flag_long>0) {
#ifdef WANT_LONGLONG_PRINTF
	  if (flag_long>1)
	    llnumber=va_arg(arg_ptr,long long);
	  else
#endif
	    number=va_arg(arg_ptr,long);
	}
	else
	  number=va_arg(arg_ptr,int);

	if (flag_in_sign) {
#ifdef WANT_LONGLONG_PRINTF
	  if ((flag_long>1)&&(llnumber<0)) {
	    llnumber=-llnumber;
	    flag_in_sign=2;
	  } else
#endif
	    if (number<0) {
	      number=-number;
	      flag_in_sign=2;
	    }
	}
	if (flag_long<0) number&=0xffff;
	if (flag_long<-1) number&=0xff;
#ifdef WANT_LONGLONG_PRINTF
	if (flag_long>1)
	  sz += __lltostr(s+sz,sizeof(buf)-5,(unsigned long long) llnumber,base,flag_upcase);
	else
#endif
	  sz += __ltostr(s+sz,sizeof(buf)-5,(unsigned long) number,base,flag_upcase);

	if (flag_in_sign==2) {
	  *(--s)='-';
	  ++sz;
	} else if ((flag_in_sign)&&(flag_sign || flag_space)) {
	  *(--s)=(flag_sign)?'+':' ';
	  ++sz;
	} else flag_in_sign=0;

	goto print_out;

#ifdef WANT_FLOATING_POINT_IN_PRINTF
      /* print a floating point value */
      case 'f':
      case 'g':
	{
	  int g=(ch=='g');
	  double d=va_arg(arg_ptr,double);
	  if (width==0) width=1;
	  if (!flag_dot) preci=6;
	  sz=__dtostr(d,buf,sizeof(buf),width,preci);
	  if (flag_dot) {
	    char *tmp;
	    if ((tmp=strchr(buf,'.'))) {
	      ++tmp;
	      while (preci>0 && *++tmp) --preci;
	      *tmp=0;
	    }
	  }
	  if (g) {
	    char *tmp,*tmp1;	/* boy, is _this_ ugly! */
	    if ((tmp=strchr(buf,'.'))) {
	      tmp1=strchr(tmp,'e');
	      while (*tmp) ++tmp;
	      if (tmp1) tmp=tmp1;
	      while (*--tmp=='0') ;
	      if (*tmp!='.') ++tmp;
	      *tmp=0;
	      if (tmp1) strcpy(tmp,tmp1);
	    }
	  }
	  preci=strlen(buf);
	  s=buf;

	  goto print_out;
	}
#endif

      default:
	break;
      }
    }
  }
  return len;
}

//link_warning("__v_printf","warning: the printf functions add several kilobytes of bloat.")


int cr7_printf(const char *format,...)
{
  int n;
  va_list arg_ptr;
  va_start(arg_ptr, format);
  n=cr7_vprintf(format, arg_ptr);
  va_end(arg_ptr);
  return n;
}

int cr7_sprintf(char *dest,const char *format,...)
{
  int n;
  va_list arg_ptr;
  va_start(arg_ptr, format);
  n=cr7_vsprintf(dest,format,arg_ptr);
  va_end (arg_ptr);
  return n;
}

//added from orignal oskit10
int cr7_snprintf(char *s, int size, const char *fmt, ...)
{
    va_list args;
    int err;
	
    va_start(args, fmt);
    err = cr7_vsnprintf(s, size, fmt, args);
    va_end(args);
    
    return err;
}
			

int __stdio_outs(const char *s,size_t len) {
#ifdef USE_OSKIT
  return console_putbytes(s,len);
#else
  return (write(1,s,len)==(int)len)?1:0;
#endif
}

int cr7_vprintf(const char *format, va_list ap)
{
  struct arg_printf _ap = { 0, (int(*)(void*,size_t,void*)) __stdio_outs };
  return __v_printf(&_ap,format,ap);
}


static int swrite(void*ptr, size_t nmemb, struct str_data* sd) {
  size_t tmp=sd->size-sd->len;
  if (tmp>0) {
    size_t len=nmemb;
    if (len>tmp) len=tmp;
    if (sd->str) {
      memcpy(sd->str+sd->len,ptr,len);
      sd->str[sd->len+len]=0;
    }
    sd->len+=len;
  }
  return nmemb;
}

int cr7_vsnprintf(char* str, size_t size, const char *format, va_list arg_ptr) {
  struct str_data sd = { str, 0, size };
  struct arg_printf ap = { &sd, (int(*)(void*,size_t,void*)) swrite };
  if (size) --sd.size;
  return __v_printf(&ap,format,arg_ptr);
}

int cr7_vsprintf(char *dest,const char *format, va_list arg_ptr)
{
  return cr7_vsnprintf(dest,(size_t)-1,format,arg_ptr);
}

//link_warning("vsprintf","warning: Avoid *sprintf; use *snprintf. It is more secure.")
