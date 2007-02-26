/**
 *	\file	dice/src/be/l4/L4BENameFactory.h
 *	\brief	contains the declaration of the class CL4BENameFactory
 *
 *	\date	02/07/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
#ifndef __DICE_L4BENAMEFACTORY_H__
#define __DICE_L4BENAMEFACTORY_H__

#include "be/BENameFactory.h"

//@{
#define STR_RESULT_VAR				0x00000001	/**< requests an IPC result variable */
#define STR_THREAD_ID_VAR			0x00000002	/**< requests a variable name for a l4thread_t variable */
#define STR_INIT_RCVSTRING_FUNC     0x00000003  /**< request name of function to init receive string */
#define STR_MSGBUF_SIZE_CONST       0x00000004  /**< const name of size dope initializer of msg buffer */
//@}

class CBEMsgBufferType;

/**	\class CL4BENameFactory
 *	\ingroup backend
 *	\brief the name factory for the back-end classes
 */
class CL4BENameFactory : public CBENameFactory
{
DECLARE_DYNAMIC(CL4BENameFactory);
// Constructor
public:
	/**	\brief constructor
	 *	\param bVerbose true if class should print status output
	 */
	CL4BENameFactory(bool bVerbose = false);
	virtual ~CL4BENameFactory();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CL4BENameFactory(CL4BENameFactory &src);

public:
    virtual String GetTypeName(int nType, bool bUnsigned, CBEContext *pContext, int nSize = 0);
    virtual String GetThreadIdVariable(CBEContext *pContext);
    virtual String GetComponentIDVariable(CBEContext *pContext);
    virtual String GetTimeoutServerVariable(CBEContext *pContext);
    virtual String GetTimeoutClientVariable(CBEContext *pContext);
    virtual String GetString(int nStringCode, CBEContext *pContext, void *pParam);
    virtual String GetResultName(CBEContext *pContext);
    virtual String GetMessageBufferMember(int nFEType, CBEContext * pContext);
    virtual String GetInitRcvStringFunction(CBEContext *pContext, String sFuncName);
	virtual String GetMsgBufferSizeDopeConst(CBEMsgBufferType* pMsgBuffer);

protected:
    virtual String GetL4TypeName(int nType, bool bUnsigned, CBEContext *pContext, int nSize);
    virtual String GetCTypeName(int nType, bool bUnsigned, CBEContext * pContext, int nSize);
};

#endif // !__DICE_L4BENAMEFACTORY_H__
