/**
 *  \file    dice/src/be/BENameFactory.h
 *  \brief   contains the declaration of the class CBENameFactory
 *
 *  \date    01/10/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "BEObject.h"
#include "BEContext.h" // for FILE_TYPE
#include <vector>
using std::vector;

class CFEBase;
class CFEInterface;
class CFEOperation;
class CFEConstDeclarator;
class CBEDeclarator;
class CBEType;
class CBEClass;
class CDeclaratorStackLocation;

/** \class CBENameFactory
 *  \ingroup backend
 *  \brief the name factory for the back-end classes
 *
 * We use seperate function for each of the different strings. The difference
 * to the previous model is that instead of using a integer identifier to
 * define the string to generate you may now use the function's name. E.g.
 * where you used GetString(STR_FILE_NAME, ...) until now you may use
 * GetFileName(...) now.  This also allows us to replace the void pointer
 * arguments use so far to transmit any kind of parameters by the explicit
 * parameters needed for each string. E.g. does GetString(STR_FILE_NAME, ...)
 * expect pParams[0] to be and int and pParams[1] to be a front-end class.
 * These two values are now: GetFileName(int, CFEBase).
 *
 * IMHO, there is no difference in writing GetString(STR_FILE_NAME,...) or
 * writing GetFileName(...). If there should be objections to this, please
 * feel free to complain. (dice\@os.inf.tu-dresden.de)
 *
 * Ok. There is one argument for using GetString(STR_FILE_NAME, ...): if you
 * wish to add some new string to a derived class, then you have to cast the
 * name factory to the derived class to be able to use this function.  E.g.:
 * add GetNewVar() to CDerivedNF then you can only use it by
 * ((CDerivedNF*)(CCompiler::GetNameFactory())->GetNewVar() but if using
 * GetString(NEW_VAR) you do CCompiler::GetNameFactory()->GetString(NEW_VAR,
 * ...).
 *
 * Because most of the functions which are present in this class became
 * simpler compared to the GetString implementation, we should stick with most
 * of them if they are used in the base class. For access to new strings in
 * derived name factories use GetString(NEW_VAR,...)
 */
class CBENameFactory : public CBEObject
{
// Constructor
  public:
    /** \brief constructor
     */
    CBENameFactory();
    virtual ~ CBENameFactory();

    virtual string GetMessageBufferMember(int nFEType);
    virtual string GetComponentIDVariable();
    virtual string GetTimeoutServerVariable();
    virtual string GetTimeoutClientVariable();
    virtual string GetScheduleClientVariable();
    virtual string GetScheduleServerVariable();
    virtual string GetMessageBufferTypeName(string sInterfaceName);
    virtual string GetMessageBufferTypeName(CFEInterface * pFEInterface);
    virtual string GetMessageBufferVariable();
    virtual string GetCorbaEnvironmentVariable();
    virtual string GetCorbaObjectVariable();
    virtual string GetString(int nStringCode,
	void *pParam = 0);
    virtual string GetInlinePrefix();
    virtual string GetOpcodeConst(CBEClass * pClass);
    virtual string GetOpcodeConst(CBEFunction * pFunction);
    virtual string GetOpcodeConst(CFEOperation * pFEOperation);
    virtual string GetSrvReturnVariable();
    virtual string GetOpcodeVariable();
    virtual string GetReplyCodeVariable();
    virtual string GetReturnVariable();
    virtual string GetTypeDefine(string sTypedefName);
    virtual string GetHeaderDefine(string sFilename);
    virtual string GetTempOffsetVariable();
    virtual string GetOffsetVariable();
    virtual string GetFunctionName(CFEInterface * pFEInterface, 
	FUNCTION_TYPE nFunctionType);
    virtual string GetFunctionName(CFEOperation * pFEOperation, 
	FUNCTION_TYPE nFunctionType);
    virtual string GetTypeName(int nType, bool bUnsigned, int nSize = 0);
    virtual string GetTypeName(CFEBase *pFERefType, string sName);
    virtual string GetFileName(CFEBase *pFEBase, FILE_TYPE nFileType);
    virtual string GetSwitchVariable();
    virtual string GetFunctionBitMaskConstant();
    virtual string GetInterfaceNumberShiftConstant();
    virtual string GetServerParameterName();
    virtual string GetIncludeFileName(CFEBase * pFEBase, FILE_TYPE nFileType);
    virtual string GetIncludeFileName(string sBaseName);
    virtual string GetMessageBufferTypeName();
    virtual string GetDummyVariable();
    virtual string GetExceptionWordVariable();
    virtual string GetConstantName(CFEConstDeclarator* pFEConstant);
    virtual string GetMessageBufferStructName(int nDirection, 
	string sFuncName, string sClassName);
    virtual string GetWordMemberVariable();
    virtual string GetWordMemberVariable(int nNumber);
    virtual string GetLocalSizeVariableName(
	vector<CDeclaratorStackLocation*> *pStack);
    virtual string GetLocalVariableName(
	vector<CDeclaratorStackLocation*> *pStack);
    virtual string GetPaddingMember(int nPadType, int nPadToType);

protected:
    virtual string GetCORBATypeName(int nType, bool bUnsigned, int nSize);
};

#endif                // !__DICE_BENAMEFACTORY_H__
