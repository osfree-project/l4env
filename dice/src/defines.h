/**
 *	\file	dice/src/defines.h 
 *	\brief	contains basic macros and definitions for all classes
 *
 *	\date	01/31/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */

/** preprocessing symbol to check header file */
#ifndef __DICE_DEFINES_H__
#define __DICE_DEFINES_H__

/** defines the maximum number of include paths */
#define MAX_INCLUDE_PATHS	25

// exceptions
#define EXCEPTION_TYPE		int		/**< defines an exception type */
#define EXCEP_BADSIZE		1		/**< defines the bad size exception */
#define EXCEP_OUTOFMEMORY	2		/**< defines the out of memory exception */

//@{
/** helper macros */
#define DWORD_ALIGN_BYTE(byte_var) ((byte_var+3) & ~0x3)
#define DWORD_FROM_BYTE(byte_var) ((byte_var+3) >> 2)
#define BYTE_FROM_DWORD(dw_var) (dw_var << 2)
#define BYTE_FROM_BIT(bit_var) ((bit_var+7) >> 4)
#define BIT_FROM_BYTE(byte_var) (byte_var << 4)
#define MAX(x,y) (((x)>(y))?(x):(y))
#define VERBOSE(s, args...) if (pContext->IsVerbose()) printf(s, ## args);
//@}

#ifdef _WIN_
#pragma warning(disable:4786)
using namespace std;
#define __PRETTY_FUNCTION__ "(no function name available)"
#endif				/* _WIN_ */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
//#include <string.h>

//@{
/** some variables used for debugging */
#ifndef GLOBAL_DEBUG_TRACE
#define GLOBAL_DEBUG_TRACE

extern int nGlobalDebug;
#define DTRACE(s, args...)	if (nGlobalDebug == 1) printf(s, ## args)
#define DTRACE_ON			nGlobalDebug = 1
#define DTRACE_OFF			nGlobalDebug = 0

#endif				/* GLOBAL_DEBUG_TRACE */
//@}

/** check condition and raise assertion if necessary */
#define ASSERT(x) { if (!(x)) { fprintf(stderr,"ASSERTION in function '%s', (file: %s, line: %i)\n",  __PRETTY_FUNCTION__, __FILE__, __LINE__); exit(1);  } }
/** check condition and raise assertion if necessary */
#define ASSERTC(x) {  if (!(x))  { fprintf(stderr, "ASSERTION in function '%s', (file: %s, line: %i) (class %s)\n",  __PRETTY_FUNCTION__, __FILE__, __LINE__, this->GetClassName());  exit(1);  } }
/** check condition and raise assertion if necessary */
#define ASSERT_KINDOF(obj,classname)	ASSERT((obj)->IsKindOf(RUNTIME_CLASS(classname)))
/** print debug information */
#define TRACE(s, args...)	printf(s, ## args)

// some dynamic class management stuff
/**\class CRuntimeClass
 *	\brief Dynamic class management stuff
 *
 * This class is used to allow dynamic class management using the DEFINE_DYNAMIC and
 * IMPLEMENT_DYNAMIC macros. This way it is possible to determine during run-time of
 * the program the class of an object. CRuntimeClass contains also information which 
 * can be used during debugging (e.g. print a class name, ...).
 */
struct CRuntimeClass
{
// Attributes
	/** \var int m_nSize
	 *	\brief the size of the class
	 */
    int m_nSize;
	/**	\var char *m_sName
	 *	\brief the name of the class
	 */
    const char *m_sName;
	/** \var CRuntimeClass *m_pBaseClasses
	 *	\brief a pointer to the base class's CRuntimeClass struct
	 */
    const CRuntimeClass **m_pBaseClasses;
  private:
	/**	\var int m_nCurArraySize
	 *	\brief contains the current size of the base class array
	 */
    int m_nCurArraySize;

  public:
// Operations
	/**
	 *	\brief checks whether object's class is derived from base-class.
	 *	\param pBaseClass a pointer to a CRuntimeClass struct
	 *	\return true if derived from base-class, false if not
	 */
  bool IsDerivedFrom(const CRuntimeClass * pBaseClass) const
  {
      ASSERT(this != 0);
      ASSERT(pBaseClass != 0);

      // am I the searched class
      if (this == pBaseClass)
          return true;

      // if we don't have base classes return false
      if (!m_pBaseClasses)
          return false;

      // iterate over base classes and ask each one of them
      for (int i = 0; i < m_nCurArraySize; i++)
      {
          if (m_pBaseClasses[i]->IsDerivedFrom(pBaseClass))
              return true;
      }
      // walked to the top, no match -> return False
      return false;
  }

  /** \brief adds a base class to this struct
   *  \param pBaseClass the base class to add
   */
  void AddBase(const CRuntimeClass * pBaseClass)
  {
      ASSERT(this != 0);
      ASSERT(pBaseClass != 0);

      // first check if we already have base class registered
      if (m_pBaseClasses != 0)
      {
          for (int i = 0; i < m_nCurArraySize; i++)
          {
              if (m_pBaseClasses[i] == pBaseClass)
                  return;
          }
      }
      // now we can add class
      m_nCurArraySize++;
      m_pBaseClasses = (const CRuntimeClass **) realloc(m_pBaseClasses, m_nCurArraySize * sizeof(CRuntimeClass *));
      m_pBaseClasses[m_nCurArraySize - 1] = pBaseClass;
  }

  /** \brief creates a runtime object and initializes it
   *  \param nSize the size of the class
   *  \param sName the name of the class
   */
  CRuntimeClass(int nSize, char *sName)
  {
      m_nSize = nSize;
      m_sName = sName;
      m_pBaseClasses = 0;
      m_nCurArraySize = 0;
  }
};

/** casts the class to the static runtime-class member */
#define RUNTIME_CLASS(class_name) ((CRuntimeClass*)(&class_name::class##class_name))

/** declares common class members */
#define DECLARE_DYNAMIC(name) \
public: \
	static CRuntimeClass class##name; \
	virtual const char* GetClassName(); \
	virtual bool IsKindOf(const CRuntimeClass* pClass) const

/** implements the common class members */
#define IMPLEMENT_DYNAMIC(name) \
struct CRuntimeClass name::class##name = CRuntimeClass(sizeof(class name), #name); \
const char* name::GetClassName() { return class##name.m_sName; } \
bool name::IsKindOf(const CRuntimeClass* pClass) const \
{ return class##name.IsDerivedFrom(pClass); }

/** implements the common class members for base classes */
#define IMPLEMENT_DYNAMIC_BASE(name, basename) \
class##name.AddBase(RUNTIME_CLASS(basename))

#endif				/* __DICE_DEFINES_H__ */
