/**
 *  \file    dice/src/be/BENameFactory.h
 *  \brief   contains the declaration of the class CBENameFactory
 *
 *  \date    01/10/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#include "BEMsgBufferType.h" // CMsgStructType
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
 * ((CDerivedNF*)(CBENameFactory::Instance())->GetNewVar() but if using
 * GetString(NEW_VAR) you do CBENameFactory::Instance()->GetString(NEW_VAR,
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

	virtual std::string GetMessageBufferMember(int nFEType);
	virtual std::string GetComponentIDVariable();
	virtual std::string GetTimeoutServerVariable(CBEFunction *pFunction);
	virtual std::string GetTimeoutClientVariable(CBEFunction *pFunction);
	virtual std::string GetScheduleClientVariable();
	virtual std::string GetScheduleServerVariable();
	virtual std::string GetMessageBufferTypeName(std::string sInterfaceName);
	virtual std::string GetMessageBufferTypeName(CFEInterface * pFEInterface);
	virtual std::string GetMessageBufferVariable();
	virtual std::string GetCorbaEnvironmentVariable();
	virtual std::string GetCorbaObjectVariable();
	virtual std::string GetString(int nStringCode,
		void *pParam = 0);
	virtual std::string GetInlinePrefix();
	virtual std::string GetOpcodeConst(CBEClass * pClass);
	virtual std::string GetOpcodeConst(CBEFunction * pFunction);
	virtual std::string GetOpcodeConst(CFEOperation * pFEOperation, bool bSecond = false);
	virtual std::string GetSrvReturnVariable();
	virtual std::string GetOpcodeVariable();
	virtual std::string GetReplyCodeVariable();
	virtual std::string GetReturnVariable();
	virtual std::string GetTypeDefine(std::string sTypedefName);
	virtual std::string GetHeaderDefine(std::string sFilename);
	virtual std::string GetTempOffsetVariable();
	virtual std::string GetOffsetVariable();
	virtual std::string GetFunctionName(CFEInterface * pFEInterface,
		FUNCTION_TYPE nFunctionType, bool bComponentSide);
	virtual std::string GetFunctionName(CFEOperation * pFEOperation,
		FUNCTION_TYPE nFunctionType, bool bComponentSide);
	virtual std::string GetTypeName(int nType, bool bUnsigned, int nSize = 0);
	virtual std::string GetTypeName(CFEBase *pFERefType, std::string sName);
	virtual std::string GetFileName(CFEBase *pFEBase, FILE_TYPE nFileType);
	virtual std::string GetSwitchVariable();
	virtual std::string GetFunctionBitMaskConstant();
	virtual std::string GetInterfaceNumberShiftConstant();
	virtual std::string GetServerParameterName();
	virtual std::string GetIncludeFileName(CFEBase * pFEBase, FILE_TYPE nFileType);
	virtual std::string GetIncludeFileName(std::string sBaseName);
	virtual std::string GetMessageBufferTypeName();
	virtual std::string GetDummyVariable(std::string sPrefix = std::string());
	virtual std::string GetExceptionWordVariable();
	virtual std::string GetConstantName(CFEConstDeclarator* pFEConstant);
	virtual std::string GetMessageBufferStructName(CMsgStructType nType,
		std::string sFuncName, std::string sClassName);
	virtual std::string GetWordMemberVariable();
	virtual std::string GetWordMemberVariable(int nNumber);
	virtual std::string GetLocalSizeVariableName(CDeclStack* pStack);
	virtual std::string GetLocalVariableName(CDeclStack* pStack);
	virtual std::string GetPaddingMember(int nPadType, int nPadToType);
	virtual std::string GetWrapperVariablePrefix();

	static CBENameFactory* Instance();

protected:
	virtual std::string GetCORBATypeName(int nType, bool bUnsigned, int nSize);

private:
    /** \var CBENameFactory *m_pInstance
     *  \brief a reference to the name factory
     */
    static CBENameFactory *m_pInstance;
};

#endif                // !__DICE_BENAMEFACTORY_H__
