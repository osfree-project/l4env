/**
 *    \file    dice/src/be/BEHeaderFile.h
 *    \brief   contains the declaration of the class CBEHeaderFile
 *
 *    \date    01/11/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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
#include <vector>
using namespace std;

class CFEFile;
class CFELibrary;
class CFEInterface;
class CFEOperation;

class CBETypedef;
class CBEType;
class CBEConstant;

/** \class CBEHeaderFile
 *  \ingroup backend
 *  \brief the header file class
 */
class CBEHeaderFile : public CBEFile
{
// Constructor
public:
    /** \brief constructor
     */
    CBEHeaderFile();
    virtual ~CBEHeaderFile();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CBEHeaderFile(CBEHeaderFile &src);

public:
    virtual void Write(CBEContext *pContext);

    virtual CBETypedef* GetNextTypedef(vector<CBETypedef*>::iterator &iter);
    virtual vector<CBETypedef*>::iterator GetFirstTypedef();
    virtual void AddTypedef(CBETypedef *pTypedef);
    virtual void RemoveTypedef(CBETypedef *pTypedef);
    virtual CBETypedef* FindTypedef(string sTypeName);

    virtual CBEConstant* GetNextConstant(vector<CBEConstant*>::iterator &iter);
    virtual vector<CBEConstant*>::iterator GetFirstConstant();
    virtual void AddConstant(CBEConstant *pConstant);
    virtual void RemoveConstant(CBEConstant *pConstant);

    virtual void AddTaggedType(CBEType *pTaggedType);
    virtual void RemoveTaggedType(CBEType *pTaggedType);
    virtual vector<CBEType*>::iterator GetFirstTaggedType();
    virtual CBEType* GetNextTaggedType(vector<CBEType*>::iterator &iter);
    virtual CBEType *FindTaggedType(string sTypeName);

    virtual bool CreateBackEnd(CFEOperation *pFEOperation, 
	CBEContext *pContext);
    virtual bool CreateBackEnd(CFEInterface *pFEInterface, 
	CBEContext *pContext);
    virtual bool CreateBackEnd(CFELibrary *pFELibrary, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEFile *pFEFile, CBEContext *pContext);
    virtual string GetIncludeFileName();

    virtual int GetSourceLineEnd();

protected:
    virtual void WriteTaggedType(CBEType *pType, CBEContext *pContext);
    virtual void WriteTypedef(CBETypedef* pTypedef, CBEContext *pContext);
    virtual void WriteConstant(CBEConstant *pConstant, CBEContext *pContext);
    virtual void WriteNameSpace(CBENameSpace *pNameSpace, CBEContext *pContext);
    virtual void WriteClass(CBEClass *pClass, CBEContext *pContext);
    virtual void WriteFunction(CBEFunction *pFunction, CBEContext *pContext);
    virtual void WriteDefaultIncludes(CBEContext * pContext);

    void CreateOrderedElementList(void);

protected:
    /** \var vector<CBEConstant*> m_vConstants
     *  \brief contains the constant declarators of the header file
     */
    vector<CBEConstant*> m_vConstants;
    /** \var vector<CBETypedef*> m_vTypedefs
     *  \brief contains the type definitions of the header file
     */
    vector<CBETypedef*> m_vTypedefs;
    /** \var vector<CBEType*> m_vTaggedTypes
     *  \brief contains the tagged types of the header files (types without typedef)
     */
    vector<CBEType*> m_vTaggedTypes;
    /** \var string m_sIncludeName
     *  \brief the file name used in include statements
     */
    string m_sIncludeName;
};

#endif // !__DICE_BEHEADERFILE_H__
