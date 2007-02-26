/**
 *	\file	dice/src/be/l4/L4BEClassFactory.h
 *	\brief	contains the declaration of the class CL4BEClassFactory
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
#ifndef __DICE_L4BEClassFactory_H__
#define __DICE_L4BEClassFactory_H__

#include "be/BEClassFactory.h"

/**	\class CL4BEClassFactory
 *	\ingroup backend
 *	\brief the class factory for the back-end classes
 *
 * We use seperate functions for each class, because the alternative is to use
 * some sort of identifier to find out which class to generate. This involves
 * writing a big switch statement.
 */
class CL4BEClassFactory : public CBEClassFactory
{
DECLARE_DYNAMIC(CL4BEClassFactory);
// Constructor
public:
	/**	\brief constructor
	 *	\param bVerbose true if class should print status output
	 */
	CL4BEClassFactory(bool bVerbose = false);
	virtual ~CL4BEClassFactory();

protected:
        /**	\brief copy constructor
         *	\param src the source to copy from
         */
        CL4BEClassFactory(CL4BEClassFactory &src);

public:
    virtual CBETestMainFunction* GetNewTestMainFunction();
    virtual CBETestServerFunction* GetNewTestServerFunction();
    virtual CBEHeaderFile* GetNewHeaderFile();
    virtual CBEWaitAnyFunction* GetNewWaitAnyFunction();
    virtual CBEUnmarshalFunction* GetNewUnmarshalFunction();
    virtual CBEMsgBufferType* GetNewMessageBufferType();
    virtual CBESrvLoopFunction* GetNewSrvLoopFunction();
    virtual CBECallFunction* GetNewCallFunction();
    virtual CBEMarshaller * GetNewMarshaller(CBEContext * pContext);
    virtual CBETestsuite * GetNewTestsuite();
    virtual CBETestFunction * GetNewTestFunction();
    virtual CBESndFunction * GetNewSndFunction();
    virtual CBERcvFunction * GetNewRcvFunction();
    virtual CBEWaitFunction * GetNewWaitFunction();
    virtual CBERcvAnyFunction * GetNewRcvAnyFunction();
    virtual CBETypedDeclarator * GetNewTypedDeclarator();
    virtual CBEReplyAnyWaitAnyFunction * GetNewReplyAnyWaitAnyFunction();
	virtual CBEReplyFunction * GetNewReplyFunction();
    virtual CBEClass * GetNewClass();
    virtual CBECommunication* GetNewCommunication();
	virtual CBEMarshalFunction* GetNewMarshalFunction();
	virtual CBEDispatchFunction* GetNewDispatchFunction();
};

#endif // !__DICE_L4BEClassFactory_H__
