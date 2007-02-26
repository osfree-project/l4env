#ifndef L4_EXEC_FILE_H
#define L4_EXEC_FILE_H

class file_t
{
  public:
    file_t();
  
    /** show message in prefixed by fname */
    void msg(const char *format, ...)
		__attribute__ ((format (printf, 2, 3)));
    int check(int error, const char *format, ...)
		__attribute__ ((format (printf, 3, 4)));
    
    /** store pointer to filename part of a pathname */
    void set_fname(const char *pathname);
    /** deliver filename reference */
    inline const char *get_fname(void)
      { return _fname; }

  private:
    const char *_fname;
};

#endif

