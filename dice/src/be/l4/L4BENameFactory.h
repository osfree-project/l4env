/**
 *  \file    dice/src/be/l4/L4BENameFactory.h
 *  \brief   contains the declaration of the class CL4BENameFactory
 *
 *  \date    02/07/2002
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
#ifndef __DICE_L4BENAMEFACTORY_H__
#define __DICE_L4BENAMEFACTORY_H__

#include "be/BENameFactory.h"

class CBETypedDeclarator;

/** \class CL4BENameFactory
 *  \ingroup backend
 *  \brief the name factory for the back-end classes
 */
class CL4BENameFactory : public CBENameFactory
{

	// Constructor
public:
	/** \brief constructor
	 */
	CL4BENameFactory();
	virtual ~CL4BENameFactory();

	/** \brief defines the value for the GetString method
	 */
	enum
	{
		STR_RESULT_VAR = 1,		/**< IPC result variable */
		STR_THREAD_ID_VAR,		/**< name for l4thread_t variable */
		STR_INIT_RCVSTRING_FUNC,	/**< init receive string */
		STR_MSGBUF_SIZE_CONST,		/**< size dope initializer */
		STR_ZERO_FPAGE,			/**< zero fpage member */
		STR_MSGTAG_VARIABLE,		/**< variable name of the MsgTag return variable */
		STR_L4_MAX			/**< maximum value for L4 */
	};

public:
	virtual std::string GetTypeName(int nType, bool bUnsigned,
		int nSize = 0);
	virtual std::string GetThreadIdVariable();
	virtual std::string GetComponentIDVariable();
	virtual std::string GetTimeoutServerVariable(CBEFunction *pFunction);
	virtual std::string GetTimeoutClientVariable(CBEFunction *pFunction);
	virtual std::string GetScheduleClientVariable();
	virtual std::string GetScheduleServerVariable();
	virtual std::string GetPartnerVariable();
	virtual std::string GetString(int nStringCode, void *pParam = 0);
	virtual std::string GetResultName();
	virtual std::string GetZeroFpage();
	virtual std::string GetMessageBufferMember(int nFEType);
	virtual std::string GetInitRcvStringFunction(std::string sFuncName);
	virtual std::string GetMsgBufferSizeDopeConst(CBETypedDeclarator* pMsgBuffer);
	virtual std::string GetPaddingMember(int nPadType, int nPadToType);
	virtual std::string GetMsgTagVariable();

protected:
	virtual std::string StripL4Fixes(std::string sName);
};

#endif // !__DICE_L4BENAMEFACTORY_H__
