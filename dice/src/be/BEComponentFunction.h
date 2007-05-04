/**
 *    \file    dice/src/be/BEComponentFunction.h
 *  \brief   contains the declaration of the class CBEComponentFunction
 *
 *    \date    01/22/2002
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
#ifndef __DICE_BECOMPONENTFUNCTION_H__
#define __DICE_BECOMPONENTFUNCTION_H__

#include "be/BEOperationFunction.h"

class CFEOperation;

/** \class CBEComponentFunction
 *  \ingroup backend
 *  \brief the function class for the back-end
 *
 * This class resembles the component function skeleton.
 */
class CBEComponentFunction : public CBEOperationFunction
{
// Constructor
public:
    /** \brief constructor
     */
    CBEComponentFunction();
    virtual ~CBEComponentFunction();

protected:
    /** \brief copy constructor */
    CBEComponentFunction(CBEComponentFunction &src);

public:
    virtual void CreateBackEnd(CFEOperation *pFEOperation);
    virtual void AddToImpl(CBEImplementationFile * pImpl);
    virtual bool IsTargetFile(CBEImplementationFile * pFile);
    virtual bool DoWriteFunction(CBEHeaderFile* pFile);
    virtual bool DoWriteFunction(CBEImplementationFile* pFile);
    virtual void WriteReturn(CBEFile *pFile);

protected:
    virtual void WriteFunctionDefinition(CBEFile* pFile);
    virtual void WriteFunctionDeclaration(CBEFile* pFile);
    virtual bool DoWriteFunctionInline(CBEFile *pFile);
    virtual void WriteUnmarshalling(CBEFile *pFile);
    virtual void SetTargetFileName(CFEBase * pFEObject);
    virtual void WriteInvocation(CBEFile *pFile);
    virtual void WriteVariableInitialization(CBEFile *pFile);
    virtual void WriteVariableDeclaration(CBEFile *pFile);
    virtual void WriteMarshalling(CBEFile * pFile);
    virtual bool DoWriteParameter(CBETypedDeclarator *pParam);
    virtual void AddAfterParameters(void);
    virtual void AddBeforeParameters(void);

    virtual bool DoTestParameter(CBETypedDeclarator *pParameter);

protected:
    /** \var CBEFunction *m_pFunction
     *  \brief a reference to the function which is tested (if we test at all)
     */
    CBEFunction *m_pFunction;
    /** \var unsigned char m_nSkipParameter
     *  \brief bitmap indication whether to skip CORBA Object or Env
     */
    unsigned char m_nSkipParameter;
};

#endif // !__DICE_BECOMPONENTFUNCTION_H__
