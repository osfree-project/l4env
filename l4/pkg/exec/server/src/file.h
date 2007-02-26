/*!
 * \file	file.h
 * \brief	
 *
 * \date	2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __FILE_H_
#define __FILE_H_

/** file class */
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

