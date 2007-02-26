/**
 *	\file	dice/src/be/BEHeaderFile.h
 *	\brief	contains the declaration of the class CBEHeaderFile
 *
 *	\date	01/11/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
#ifndef __DICE_BEHEADERFILE_H__
#define __DICE_BEHEADERFILE_H__

#include "be/BEFile.h"

class CFEFile;
class CFELibrary;
class CFEInterface;
class CFEOperation;

class CBETypedef;
class CBEType;
class CBEConstant;

/**	\class CBEHeaderFile
 *	\ingroup backend
 *	\brief the header file class
 */
class CBEHeaderFile : public CBEFile
{
DECLARE_DYNAMIC(CBEHeaderFile);
// Constructor
public:
	/**	\brief constructor
	 */
	CBEHeaderFile();
	virtual ~CBEHeaderFile();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CBEHeaderFile(CBEHeaderFile &src);

public:
	virtual void Write(CBEContext *pContext);

	virtual CBETypedef* GetNextTypedef(VectorElement* &pIter);
	virtual VectorElement* GetFirstTypedef();
	virtual void AddTypedef(CBETypedef *pTypedef);
	virtual void RemoveTypedef(CBETypedef *pTypedef);
	virtual CBETypedef* FindTypedef(String sTypeName);

	virtual CBEConstant* GetNextConstant(VectorElement* &pIter);
	virtual VectorElement* GetFirstConstant();
	virtual void AddConstant(CBEConstant *pConstant);
	virtual void RemoveConstant(CBEConstant *pConstant);

	virtual void AddTaggedType(CBEType *pTaggedType);
	virtual void RemoveTaggedType(CBEType *pTaggedType);
	virtual VectorElement* GetFirstTaggedType();
	virtual CBEType* GetNextTaggedType(VectorElement* &pIter);
	virtual CBEType *FindTaggedType(String sTypeName);

	virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);
	virtual bool CreateBackEnd(CFEInterface *pFEInterface, CBEContext *pContext);
	virtual bool CreateBackEnd(CFELibrary *pFELibrary, CBEContext *pContext);
	virtual bool CreateBackEnd(CFEFile *pFEFile, CBEContext *pContext);
    virtual String GetIncludeFileName();

protected:
    virtual void WriteTaggedTypes(CBEContext *pContext);
	virtual void WriteTypedefs(CBEContext *pContext);
	virtual void WriteConstants(CBEContext *pContext);
	virtual void WriteNameSpaces(CBEContext * pContext);
	virtual void WriteClasses(CBEContext *pContext);
	virtual void WriteFunctions(CBEContext * pContext);
    virtual void WriteIncludesBeforeTypes(CBEContext * pContext);

protected:
	/**	\var Vector m_vConstants
	 *	\brief contains the constant declarators of the header file
	 */
	Vector m_vConstants;
	/**	\var Vector m_vTypedefs
	 *	\brief contains the type definitions of the header file
	 */
	Vector m_vTypedefs;
	/** \var Vector m_vTaggedTypes
	 *  \brief contains the tagged types of the header files (types without typedef)
	 */
	Vector m_vTaggedTypes;
    /** \var String m_sIncludeName
     *  \brief the file name used in include statements
     */
    String m_sIncludeName;
};

#endif // !__DICE_BEHEADERFILE_H__
