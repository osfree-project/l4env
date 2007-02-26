/**
 *	\file	dice/src/be/BEObject.h 
 *	\brief	contains the declaration of the class CBEObject
 *
 *	\date	02/13/2001
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
#ifndef __DICE_FE_BEOBJECT_H__
#define __DICE_FE_BEOBJECT_H__

#include "Object.h"
#include "CString.h"

class CBERoot;
class CBETarget;
class CBEFunction;
class CBEStructType;
class CBEUnionType;
class CBEFile;
class CFEBase;
class CBEContext;
class CBEHeaderFile;
class CBEImplementationFile;

/** \class CBEObject
 *	\ingroup backend
 *	\brief The back-end base class
 *
 * This class is the base class for all classes of the back-end. It
 * implements several features, each back-end class might use.
 */
class CBEObject : public CObject
{
DECLARE_DYNAMIC(CBEObject);

// standard constructor/destructor
public:
	/** constructs a back-end base object
	 *	\param pParent the parent object of this one */
	CBEObject(CObject* pParent = 0);
    virtual ~CBEObject();

protected:
    /**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CBEObject(CBEObject &src);

// Operations
public:
	virtual CBEUnionType* GetUnionType();
	virtual CBEStructType* GetStructType();
	virtual CBEFunction* GetFunction();
    virtual CObject * Clone();
	virtual CBERoot* GetRoot();
	virtual bool IsTargetFile(CBEImplementationFile *pFile);
	virtual bool IsTargetFile(CBEHeaderFile *pFile);

protected:
	virtual void SetTargetFileName(CFEBase *pFEObject, CBEContext *pContext);

protected:
	/** \var String m_sTargetHeader
	 *  \brief contains the calculated target file name  for the header file
	 */
	String m_sTargetHeader;
	/** \var String m_sTargetImplementation
	 *  \brief contains the calculated target file name  for the implementation file
	 */
	String m_sTargetImplementation;
	/** \var String m_sTargetTestsuite
	 *  \brief contains the calculated target file name  for the test-suite file
	 */
	String m_sTargetTestsuite;
};

#endif // !__DICE_FE_BEOBJECT_H__
