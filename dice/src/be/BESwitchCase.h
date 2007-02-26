/**
 *	\file	dice/src/be/BESwitchCase.h
 *	\brief	contains the declaration of the class CBESwitchCase
 *
 *	\date	01/29/2002
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BESWITCHCASE_H__
#define __DICE_BESWITCHCASE_H__

#include "be/BEOperationFunction.h"

class CBEUnmarshalFunction;
class CBEComponentFunction;
class CBEFunction;

/**	\class CBESwitchCase
 *	\ingroup backend
 *	\brief the function class for the back-end
 *
 * This class contains resembles a back-end function which belongs to a front-end operation
 */
class CBESwitchCase : public CBEOperationFunction  
{
DECLARE_DYNAMIC(CBESwitchCase);
// Constructor
public:
	/**	\brief constructor
	 */
	CBESwitchCase();
	virtual ~CBESwitchCase();

protected:
	/**	\brief copy constructor */
	CBESwitchCase(CBESwitchCase &src);

public:
    virtual void Write(CBEFile *pFile, CBEContext *pContext);
    virtual bool CreateBackEnd(CFEOperation *pFEOperation, CBEContext *pContext);
    virtual void SetMessageBufferType(CBEContext *pContext);
    virtual void SetCallVariable(String sOriginalName, int nStars, String sCallName, CBEContext * pContext);

protected:
    virtual void WriteVariableInitialization(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteVariableDeclaration(CBEFile *pFile, CBEContext *pContext);
    virtual void WriteCleanup(CBEFile *pFile, CBEContext *pContext);

protected:
	/**	\var String m_sOpcode
	 *	\brief the opcode constant
	 */
	String m_sOpcode;
	/**	\var CBEUnmarshalFunction *m_pUnmarshalFunction
	 *	\brief a reference to the corresponding unmarshal function
	 */
	CBEUnmarshalFunction *m_pUnmarshalFunction;
	/**	\var CBEFunction *m_pReplyWaitFunction
	 *	\brief a reference to the corresponding reply-and-wait or wait-any function
	 */
	CBEFunction *m_pReplyWaitFunction;
	/**	\var CBEComponentFunction *m_pComponentFunction
	 *	\brief a reference to the corresponding component function
	 */
	CBEComponentFunction *m_pComponentFunction;
};

#endif // !__DICE_BESWITCHCASE_H__
