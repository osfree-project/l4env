/**
 *    \file    dice/src/be/BENameFactory.h
 *    \brief   contains the declaration of the class CBENameFactory
 *
 *    \date    01/10/2002
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
#ifndef __DICE_BENAMEFACTORY_H__
#define __DICE_BENAMEFACTORY_H__

#include "be/BEObject.h"

class CFEBase;
class CFEInterface;
class CFEOperation;
class CFEConstDeclarator;
class CBEContext;
class CBEDeclarator;
class CBEType;
class CBEClass;

/** \class CBENameFactory
 *  \ingroup backend
 *  \brief the name factory for the back-end classes
 *
 * We use seperate function for each of the different strings. The difference to the previous model is that
 * instead of using a integer identifier to define the string to generate you may now use the function's
 * name. E.g. where you used GetString(STR_FILE_NAME, ...) until now you may use GetFileName(...) now.
 * This also allows us to replace the void pointer arguments use so far to transmit any kind of parameters
 * by the explicit parameters needed for each string. E.g. does GetString(STR_FILE_NAME, ...) expect
 * pParams[0] to be and int and pParams[1] to be a front-end class. These two values are now:
 * GetFileName(int, CFEBase, CBEContext).
 *
 * IMHO, there is no difference in writing GetString(STR_FILE_NAME,...) or writing GetFileName(...). If
 * there should be objections to this, please feel free to complain. (dice\@os.inf.tu-dresden.de)
 *
 * Ok. There is one argument for using GetString(STR_FILE_NAME, ...): if you wish to add some new string
 * to a derived class, then you have to cast the name factory to the derived class to be able to use this
 * function.
 * E.g.: add GetNewVar() to CDerivedNF then you can only use it by
 * ((CDerivedNF*)(pContext->GetNameFactory())->GetNewVar()
 * but if using GetString(NEW_VAR) you do pContext->GetNameFactory()->GetString(NEW_VAR, ...).
 *
 * Because most of the functions which are present in this class became simpler compared to the GetString
 * implementation, we should stick with most of them if they are used in the base class. For access to new
 * strings in derived name factories use GetString(NEW_VAR,...)
 */
class CBENameFactory:public CBEObject
{
// Constructor
  public:
    /**    \brief constructor
     *    \param bVerbose true if class should print status output
     */
    CBENameFactory(bool bVerbose = false);
    virtual ~ CBENameFactory();

protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CBENameFactory(CBENameFactory & src);

public:
    virtual string GetMessageBufferMember(CBEType * pType, CBEContext * pContext);
    virtual string GetMessageBufferMember(int nFEType, CBEContext * pContext);
    virtual string GetGlobalReturnVariable(CBEFunction * pFunction, CBEContext * pContext);
    virtual string GetGlobalTestVariable(CBEDeclarator * pParameter, CBEContext * pContext);
    virtual string GetComponentIDVariable(CBEContext * pContext);
    virtual string GetTimeoutServerVariable(CBEContext * pContext);
    virtual string GetTimeoutClientVariable(CBEContext * pContext);
    virtual string GetScheduleClientVariable(CBEContext * pContext);
    virtual string GetMessageBufferTypeName(string sInterfaceName, CBEContext * pContext);
    virtual string GetMessageBufferTypeName(CFEInterface * pFEInterface, CBEContext * pContext);
    virtual string GetMessageBufferVariable(CBEContext * pContext);
    virtual string GetCorbaEnvironmentVariable(CBEContext * pContext);
    virtual string GetCorbaObjectVariable(CBEContext * pContext);
    virtual string GetString(int nStringCode, CBEContext * pContext, void *pParam = 0);
    virtual string GetInlinePrefix(CBEContext * pContext);
    virtual string GetOpcodeConst(CBEClass * pClass, CBEContext * pContext);
    virtual string GetOpcodeConst(CBEFunction * pFunction, CBEContext * pContext);
    virtual string GetOpcodeConst(CFEOperation * pFEOperation, CBEContext * pContext);
    virtual string GetSrvReturnVariable(CBEContext * pContext);
    virtual string GetOpcodeVariable(CBEContext * pContext);
    virtual string GetReplyCodeVariable(CBEContext * pContext);
    virtual string GetReturnVariable(CBEContext * pContext);
    virtual string GetTypeDefine(string sTypedefName, CBEContext * pContext);
    virtual string GetHeaderDefine(string sFilename, CBEContext * pContext);
    virtual string GetTempOffsetVariable(CBEContext * pContext);
    virtual string GetOffsetVariable(CBEContext * pContext);
    virtual string GetFunctionName(CFEInterface * pFEInterface, CBEContext * pContext);
    virtual string GetFunctionName(CFEOperation * pFEOperation, CBEContext * pContext);
    virtual string GetFunctionName(int nFunctionType, CBEContext *pContext);
    virtual string GetTypeName(int nType, bool bUnsigned, CBEContext * pContext, int nSize = 0);
    virtual string GetTypeName(CFEBase *pFERefType, string sName, CBEContext *pContext);
    virtual string GetFileName(CFEBase *pFEBase, CBEContext * pContext);
    virtual string GetSwitchVariable();
    virtual string GetFunctionBitMaskConstant();
    virtual string GetInterfaceNumberShiftConstant();
    virtual string GetServerParameterName();
    virtual string GetIncludeFileName(CFEBase * pFEBase, CBEContext * pContext);
    virtual string GetIncludeFileName(string sBaseName, CBEContext* pContext);
    virtual string GetMessageBufferTypeName(CBEContext *pContext);
    virtual string GetDummyVariable(CBEContext* pContext);
    virtual string GetExceptionWordVariable(CBEContext *pContext);
    virtual string GetConstantName(CFEConstDeclarator* pFEConstant, CBEContext* pContext);

protected:
    virtual string GetCTypeName(int nType, bool bUnsigned, CBEContext *pContext, int nSize);

protected:
    /**    \var bool m_bVerbose
     *    \brief is true if this class should output verbose stuff
     */
     bool m_bVerbose;
};

#endif                // !__DICE_BENAMEFACTORY_H__
