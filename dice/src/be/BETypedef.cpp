/**
 *	\file	dice/src/be/BETypedef.cpp
 *	\brief	contains the implementation of the class CBETypedef
 *
 *	\date	01/18/2002
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

#include "be/BETypedef.h"
#include "be/BEContext.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEDeclarator.h"
#include "be/BEStructType.h"

#include "fe/FETypedDeclarator.h"

IMPLEMENT_DYNAMIC(CBETypedef);

CBETypedef::CBETypedef()
{
    m_pAlias = 0;
    IMPLEMENT_DYNAMIC_BASE(CBETypedef, CBETypedDeclarator);
}

CBETypedef::CBETypedef(CBETypedef & src)
: CBETypedDeclarator(src)
{
    m_pAlias = src.m_pAlias;
    m_sDefine = src.m_sDefine;
    IMPLEMENT_DYNAMIC_BASE(CBETypedef, CBETypedDeclarator);
}

/**	\brief destructor of this instance */
CBETypedef::~CBETypedef()
{

}

/**	\brief writes the definition of a type to the target file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The reason for CBETypedef to exist is to write the define symbols around the type definition.
 */
void CBETypedef::WriteDeclaration(CBEHeaderFile * pFile, CBEContext * pContext)
{
    bool bNeedDefine = !m_sDefine.IsEmpty();
    if (bNeedDefine)
    {
        pFile->Print("#if !defined(%s)\n", (const char *) m_sDefine);
        pFile->Print("#define %s\n", (const char *) m_sDefine);
    }

    int nSize = GetSize();
    pFile->Print("/* size = %d bytes == %d dwords */\n", nSize, (nSize+3) >> 2);

    pFile->Print("typedef ");
    CBETypedDeclarator::WriteDeclaration(pFile, pContext);
    pFile->Print(";\n");

    if (bNeedDefine)
    {
        pFile->Print("#endif /* %s */\n\n", (const char *) m_sDefine);
    }
}

/**	\brief creates the typedef class
 *	\param pFETypedef the corresponding type definition
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This implementation extracts the define symbol.
 */
bool CBETypedef::CreateBackEnd(CFETypedDeclarator * pFETypedef, CBEContext * pContext)
{
    // set target file name
    SetTargetFileName(pFETypedef, pContext);
    // if declarator changes, we have to reset the alias as well
    m_pAlias = 0;

    if (!CBETypedDeclarator::CreateBackEnd(pFETypedef, pContext))
        return false;

    // a typedef can have only one name
    CBEDeclarator *pDecl = GetAlias();
    String sAlias = pContext->GetNameFactory()->GetTypeName(pFETypedef, pDecl->GetName(), pContext);
    // recreate decl
    pDecl->CreateBackEnd(sAlias, pDecl->GetStars(), pContext);
    m_sDefine = pContext->GetNameFactory()->GetTypeDefine(pDecl->GetName(), pContext);

    return true;
}

/** \brief adds a typedefinition to the header file
 *  \param pHeader the header file to be added to
 *  \param pContext the context of this creation
 *  \return true if successful
 *
 * A type definition is usually always added to a header file, if it is the
 * respective target file (for the IDL file).
 */
bool CBETypedef::AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext)
{
    if (IsTargetFile(pHeader))
        pHeader->AddTypedef(this);
    return true;
}

/** \brief retrieves the alias type name
 *  \return a reference to the declarator containing the alias
 *
 * The alias of a type is the only name of the typedef.
 */
CBEDeclarator* CBETypedef::GetAlias()
{
    if (!m_pAlias)
    {
        VectorElement *pIter = GetFirstDeclarator();
        m_pAlias = GetNextDeclarator(pIter);
    }
    return m_pAlias;
}

/** \brief creates a new instance of this object */
CObject * CBETypedef::Clone()
{
    return new CBETypedef(*this);
}
