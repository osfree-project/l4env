/**
 *    \file    dice/src/be/BETestFunction.h
 *    \brief   contains the declaration of the class CBETestFunction
 *
 *    \date    03/08/2002
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
#ifndef __DICE_BETESTFUNCTION_H__
#define __DICE_BETESTFUNCTION_H__

#include "be/BEOperationFunction.h"
#include <vector>
using namespace std;

class CFEInterface;
class CFEOperation;
class CBEContext;
class CBEExpression;
class CDeclaratorStackLocation;

/**    \class CBETestFunction
 *    \ingroup backend
 *    \brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a front-end operation
 */
class CBETestFunction : public CBEOperationFunction
{
// Constructor
public:
    /**    \brief constructor
     */
    CBETestFunction();
    virtual ~CBETestFunction();

protected:
    /**    \brief copy constructor */
    CBETestFunction(CBETestFunction &src);

public:
    virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual bool DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext);
    virtual bool DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext);
    virtual void CompareVariable(CBEFile *pFile, CBETypedDeclarator *pParameter, CBEContext *pContext);
    virtual void InitLocalVariable(CBEFile *pFile, CBETypedDeclarator *pParameter, CBEContext *pContext);
    virtual bool IsTargetFile(CBEImplementationFile * pFile);
    virtual bool DoWriteFunction(CBEHeaderFile* pFile,  CBEContext* pContext);
    virtual bool DoWriteFunction(CBEImplementationFile* pFile,  CBEContext* pContext);
    virtual bool HasAdditionalReference(CBEDeclarator * pDeclarator, CBEContext * pContext, bool bCall);

protected:
    virtual void WriteGlobalVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteOutVariableCheck(CBEFile *pFile, CBETypedDeclarator *pParameter, CBEContext *pContext);
    virtual void WriteCleanup(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteInvocation(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableInitialization(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteFunctionDefinition(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteModulo(CBEFile *pFile, CBEContext *pContext);

    virtual void InitGlobalVariable(CBEFile * pFile, CBETypedDeclarator * pParameter, CBEContext * pContext);
    virtual void InitGlobalDeclarator(CBEFile * pFile, CBEType * pType, CBEContext * pContext);
    virtual void InitGlobalArray(CBEFile *pFile, CBEType *pType, CBEContext *pContext);
    virtual void InitGlobalConstArray(CBEFile *pFile, CBEType *pType,
        vector<CBEExpression*>::iterator iter,
        int nLevel,
        CBEContext *pContext);
    virtual void InitGlobalVarArray(CBEFile *pFile, CBEType *pType,
        vector<CBEExpression*>::iterator iterB,
        int nLevel,
        CBEAttribute *pSizeAttr,
        vector<CBEDeclarator*>::iterator iterAttr,
        CBEContext *pContext);
    virtual void InitGlobalUnion(CBEFile *pFile, CBEUnionType *pType, CBEContext *pContext);
    virtual void InitGlobalStruct(CBEFile *pFile, CBEStructType *pType, CBEContext *pContext);
    virtual void InitGlobalString(CBEFile * pFile, CBEType * pType, CBEContext * pContext);

    virtual void InitLocalDeclarator(CBEFile * pFile, CBEType * pType, CBEContext * pContext);
    virtual void InitLocalArray(CBEFile *pFile, CBEType *pType, CBEContext *pContext);
    virtual void InitLocalConstArray(CBEFile *pFile, CBEType *pType,
        vector<CBEExpression*>::iterator iter,
        int nLevel,
        CBEContext *pContext);
    virtual void InitLocalVarArray(CBEFile *pFile, CBEType *pType,
        vector<CBEExpression*>::iterator iterB,
        int nLevel,
        CBEAttribute *pSizeAttr,
        vector<CBEDeclarator*>::iterator iterAttr,
        CBEContext *pContext);
    virtual void InitLocalUnion(CBEFile *pFile, CBEUnionType *pType, CBEContext *pContext);
    virtual void InitLocalStruct(CBEFile *pFile, CBEStructType *pType, CBEContext *pContext);

    virtual void InitPreallocVariable(CBEFile *pFile, CBETypedDeclarator *pParameter, CBEContext *pContext);
    virtual void FreePreallocVariable(CBEFile *pFile, CBETypedDeclarator *pParameter, CBEContext *pContext);

    virtual void CompareDeclarator(CBEFile *pFile, CBEType *pType, vector<CDeclaratorStackLocation*> *pStack, CBEContext *pContext);
    virtual void CompareStruct(CBEFile *pFile, CBEStructType *pType, vector<CDeclaratorStackLocation*> *pStack, CBEContext *pContext);
    virtual void CompareArray(CBEFile *pFile, CBEType *pType, vector<CDeclaratorStackLocation*> *pStack, CBEContext *pContext);
    virtual void CompareConstArray(CBEFile *pFile, CBEType *pType,
        vector<CDeclaratorStackLocation*> *pStack,
        vector<CBEExpression*>::iterator iterB,
        int nLevel,
        CBEContext *pContext);
    virtual void CompareVarArray(CBEFile *pFile, CBEType *pType,
        vector<CDeclaratorStackLocation*> *pStack,
        vector<CBEExpression*>::iterator iterB,
        int nLevel,
        CBEAttribute *pSizeAttr,
        vector<CBEDeclarator*>::iterator iterAttr,
        CBEContext *pContext);
    virtual void CompareUnion(CBEFile *pFile, CBEUnionType *pType, vector<CDeclaratorStackLocation*> *pStack, CBEContext *pContext);
    virtual void CompareVariable(CBEFile * pFile, CBETypedDeclarator * pParameter, vector<CDeclaratorStackLocation*> *pStack, CBEContext * pContext);
    virtual void CompareString(CBEFile * pFile, CBEType * pType, vector<CDeclaratorStackLocation*> * pStack, CBEContext * pContext);

    virtual void WriteComparison(CBEFile *pFile, vector<CDeclaratorStackLocation*> *pStack, CBEContext *pContext);
    virtual void WriteErrorMessage(CBEFile * pFile, vector<CDeclaratorStackLocation*> * pStack, CBEContext * pContext);
    virtual void SetTargetFileName(CFEBase * pFEObject, CBEContext * pContext);
    virtual void WriteAbs(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteMarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext);
    virtual void WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext);
    virtual void WriteSuccessMessage(CBEFile *pFile, vector<CDeclaratorStackLocation*> *pStack, CBEContext *pContext);

protected:
    /**    \var CBEFunction *m_pFunction
     *    \brief a reference to the function we test
     */
    CBEFunction *m_pFunction;
    /** \var vector<CDeclaratorStackLocation*> m_vDeclaratorStack
     *  \brief declarator stack - used when intializing global variables
     */
    vector<CDeclaratorStackLocation*> m_vDeclaratorStack;
    /** \var CBETypedDeclarator m_pParameter
     *  \brief a reference to the currently initialized parameter
     */
    CBETypedDeclarator *m_pParameter;
};

#endif // !__DICE_BETESTFUNCTION_H__
