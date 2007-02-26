/**
 *    \file    dice/src/be/BEConstant.cpp
 *    \brief   contains the implementation of the class CBEConstant
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

#include "be/BEConstant.h"
#include "be/BEContext.h"
#include "be/BEExpression.h"
#include "be/BEType.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"

#include "fe/FEConstDeclarator.h"
#include "fe/FETypeSpec.h"

CBEConstant::CBEConstant()
{
    m_pType = 0;
    m_pValue = 0;
    m_bAlwaysDefine = false;
}

CBEConstant::CBEConstant(CBEConstant & src)
: CBEObject(src)
{
    m_sName = src.m_sName;
    m_bAlwaysDefine = src.m_bAlwaysDefine;
    m_pType = (CBEType*)src.m_pType->Clone();
    m_pType->SetParent(this);
    m_pValue = (CBEExpression*)src.m_pValue->Clone();
    m_pValue->SetParent(this);
}

/**    \brief destructor of this instance */
CBEConstant::~CBEConstant()
{
    if (m_pType)
        delete m_pType;
    if (m_pValue)
        delete m_pValue;
}

/**    \brief creates the back-end constant declarator
 *    \param *pFEConstDeclarator therespective front-end declarator
 *    \param *pContext the context of the code generation
 *    \return true if the code generation was successful
 *
 * This implementation sets the name, the type and the value of the constant.
 */
bool CBEConstant::CreateBackEnd(CFEConstDeclarator * pFEConstDeclarator, CBEContext * pContext)
{
    // call CBEObject's CreateBackEnd method
    if (!CBEObject::CreateBackEnd(pFEConstDeclarator))
        return false;

    // set target file name
    SetTargetFileName(pFEConstDeclarator, pContext);
    // get name
    m_sName = pContext->GetNameFactory()->GetConstantName(pFEConstDeclarator, pContext);
    // get type
    CFETypeSpec *pFEType = pFEConstDeclarator->GetType();
    m_pType = pContext->GetClassFactory()->GetNewType(pFEType->GetType());
    m_pType->SetParent(this);
    if (!m_pType->CreateBackEnd(pFEType, pContext))
    {
        delete m_pType;
        m_pType = 0;
        return false;
    }
    // check for constant's value
    if (!pFEConstDeclarator->GetValue())
    {
        VERBOSE("CBEConstant::CreateBackEnd for \"%s\" failed because no value\n", m_sName.c_str());
        return false;
    }
    // get value
    m_pValue = pContext->GetClassFactory()->GetNewExpression();
    m_pValue->SetParent(this);
    if (!m_pValue->CreateBackEnd(pFEConstDeclarator->GetValue(), pContext))
    {
        delete m_pValue;
        m_pValue = 0;
        return false;
    }

    return true;
}

/**    \brief creates the back-end constants declarator
 *    \param pType the type of the constant
 *    \param sName the name of the constant
 *    \param pValue the expression representing the value of the constant
 *    \param bAlwaysDefine true if the const chould always appear as a define
 *    \param pContext the context of the generation
 *    \return true if successful.
 */
bool CBEConstant::CreateBackEnd(CBEType * pType, string sName, CBEExpression * pValue, bool bAlwaysDefine, CBEContext * pContext)
{
     m_sName = sName;
     m_bAlwaysDefine = bAlwaysDefine;
     m_pType = pType;
     if (m_pType)
         m_pType->SetParent(this);
     else
         return false;
     m_pValue = pValue;
     if (m_pValue)
         m_pValue->SetParent(this);
     else
         return false;
     return true;
}

/**    \brief writes the constant definition to the header file
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * A constant definitionlooks like this:
 *
 * <code>#define &lt;name&gt; &lt;expression&gt;</code>
 *
 * So we first write the define, then the name, which is the same as used in the IDL file,
 * and then the expression.
 */
void CBEConstant::Write(CBEHeaderFile * pFile, CBEContext * pContext)
{
    if (!pFile->IsOpen())
        return;

    *pFile << "#ifndef _constdef_" << m_sName << "\n";
    *pFile << "#define _constdef_" << m_sName << "\n";
    if (pContext->IsOptionSet(PROGRAM_CONST_AS_DEFINE) || m_bAlwaysDefine)
    {
        // #define <name>
        *pFile << "#define " << m_sName << " ";
        // <expression>
        m_pValue->Write(pFile, pContext);
        // newline
        *pFile << "\n";
    }
    else
    {
        // should be static
        *pFile << "const ";
        m_pType->Write(pFile, pContext);
        *pFile << " " << GetName() << " = ";
        m_pValue->Write(pFile, pContext);
        *pFile << ";\n";
    }
    *pFile << "#endif /* _constdef_" << m_sName << " */\n";
}

/** \brief returns the name of the constant
 *  \return the name of the constant
 */
string CBEConstant::GetName()
{
    return m_sName;
}

/** \brief return the value of this constant
 *  \return a reference to the expression
 */
CBEExpression *CBEConstant::GetValue()
{
    return m_pValue;
}

/** \brief checks if this constant is added to the header file
 *  \param pHeader the header file to be added to
 *  \param pContext the context of this create process
 *  \return if const could be added successfully
 *
 * A const usually is always added to a header file , if the target file is the
 * respective file for the IDL file
 */
bool CBEConstant::AddToFile(CBEHeaderFile *pHeader, CBEContext *pContext)
{
    VERBOSE("CBEConstant::AddToFile(header: %s) for const %s called\n",
        pHeader->GetFileName().c_str(), m_sName.c_str());
    if (IsTargetFile(pHeader))
        pHeader->AddConstant(this);
    return true;
}
