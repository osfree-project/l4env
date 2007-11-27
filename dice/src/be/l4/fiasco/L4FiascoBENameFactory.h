/**
 *  \file    dice/src/be/l4/fiasco/L4FiascoBENameFactory.h
 *  \brief   contains the declaration of the class CL4FiascoBENameFactory
 *
 *  \date    08/24/2007
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
#ifndef L4FIASCOBENAMEFACTORY_H
#define L4FIASCOBENAMEFACTORY_H

#include <be/l4/L4BENameFactory.h>

/** \class CL4FiascoBENameFactory
 *  \ingroup backend
 *  \brief contains functions to create V4 specific names
 */
class CL4FiascoBENameFactory : public CL4BENameFactory
{

public:
    /** \brief creates the instance of the name factory
     */
    CL4FiascoBENameFactory();
    virtual ~CL4FiascoBENameFactory();

	/** \brief contains the L4.Fiasco specific values for the GetString function
	 */
	enum
	{
		STR_L4FIASCO_BASE = STR_L4_MAX, /**< ensure disjunct values */
		STR_UTCB_INITIALIZER,    /**< string with initializer for UTCB address */
		STR_L4FIASCO_MAX                /**< maximum L4V4 value */
	};

	virtual std::string GetString(int nStringCode, void *pParam = 0);
    virtual std::string GetTypeName(int nType, bool bUnsigned, int nSize = 0);
    virtual std::string GetMsgTagVariable();
	virtual std::string GetUtcbInitializer(CBEFunction *pFunction);
    virtual std::string GetTimeoutServerVariable(CBEFunction *pFunction);
    virtual std::string GetTimeoutClientVariable(CBEFunction *pFunction);
};

#endif
