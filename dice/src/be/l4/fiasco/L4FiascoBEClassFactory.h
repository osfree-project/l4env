/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBEClassFactory.h
 *  \brief   contains the declaration of the class CL4FiascoBEClassFactory
 *
 *  \date    08/20/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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
#ifndef __DICE_L4FIASCOBECLASSFACTORY_H__
#define __DICE_L4FIASCOBECLASSFACTORY_H__

#include "be/l4/L4BEClassFactory.h"

/** \class CL4FiascoBEClassFactory
 *  \ingroup backend
 *  \brief the class factory for the back-end classes
 *
 * We use seperate functions for each class, because the alternative is to use
 * some sort of identifier to find out which class to generate. This involves
 * writing a big switch statement.
 */
class CL4FiascoBEClassFactory : public CL4BEClassFactory
{
// Constructor
public:
    /** \brief constructor
     */
    CL4FiascoBEClassFactory();
    ~CL4FiascoBEClassFactory();

public:
    virtual CBESizes * GetNewSizes();
    virtual CBECommunication* GetNewCommunication();
    virtual CBEMsgBuffer* GetNewMessageBuffer();
    virtual CBEClass* GetNewClass();
    virtual CBEDispatchFunction* GetNewDispatchFunction();
    virtual CBECallFunction* GetNewCallFunction();
    virtual CBESrvLoopFunction* GetNewSrvLoopFunction();
    virtual CBEWaitAnyFunction* GetNewWaitAnyFunction();
    virtual CBEWaitAnyFunction* GetNewRcvAnyFunction();
    virtual CBEWaitAnyFunction* GetNewReplyAnyWaitAnyFunction();
    virtual CBEWaitFunction * GetNewRcvFunction();
    virtual CBEWaitFunction * GetNewWaitFunction();
	virtual CBEUnmarshalFunction* GetNewUnmarshalFunction();
	virtual CBEMarshalFunction* GetNewMarshalFunction();
	virtual CBESndFunction* GetNewSndFunction();
	virtual CBEReplyFunction* GetNewReplyFunction();
};

#endif // !__DICE_L4FIASCOBECLASSFACTORY_H__
