/**
 *    \file    dice/src/be/BETypedef.h
 *    \brief   contains the declaration of the class CBETypedef
 *
 *    \date    01/18/2002
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
#ifndef __DICE_BETYPEDEF_H__
#define __DICE_BETYPEDEF_H__

#include "be/BETypedDeclarator.h"

class CFEUnionCase;
class CBEContext;

/**    \class CBETypedef
 *    \ingroup backend
 *    \brief the back-end type definition
 */
class CBETypedef : public CBETypedDeclarator
{

// Constructor
public:
    /**    \brief constructor
     */
    CBETypedef();
    virtual ~CBETypedef();

protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CBETypedef(CBETypedef &src);

public:
    virtual bool CreateBackEnd(CFETypedDeclarator *pFETypedef, CBEContext *pContext);
    virtual bool CreateBackEnd(CBEType * pType, string sName, CFEBase *pFERefObject, CBEContext * pContext);
    virtual void WriteDeclaration(CBEHeaderFile *pFile, CBEContext *pContext);
    virtual bool AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext);
    virtual CBEDeclarator* GetAlias();
    virtual CObject * Clone();

protected:
    /**    \var string m_sDefine
     *    \brief the define symbol to brace the type definition
     */
    string m_sDefine;
private:
    /** \var CBEDeclarator *m_pAlias
     *  \brief a 'shortcut' reference to the alias name
     */
    CBEDeclarator *m_pAlias;
};

#endif // !__DICE_BETYPEDEF_H__
