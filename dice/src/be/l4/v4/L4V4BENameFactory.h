/**
 *  \file    dice/src/be/l4/v4/L4V4BENameFactory.h
 *  \brief   contains the declaration of the class CL4V4BENameFactory
 *
 *  \date    01/08/2004
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
#ifndef L4V4BENAMEFACTORY_H
#define L4V4BENAMEFACTORY_H

#include <be/l4/L4BENameFactory.h>

/** \class CL4V4BENameFactory
 *  \ingroup backend
 *  \brief contains functions to create V4 specific names
 */
class CL4V4BENameFactory : public CL4BENameFactory
{

public:
	/** \brief creates the instance of the name factory
	 */
	CL4V4BENameFactory();
	virtual ~CL4V4BENameFactory();

	/** \brief contains the L4 V4 specific values for the GetString function
	 */
	enum
	{
		STR_L4V4_BASE = STR_L4_MAX, /**< ensure disjunct values */
		STR_INIT_RCVSTR_VARIABLE,   /**< variable to hold string length for init-rcvstr call */
		STR_L4V4_MAX                /**< maximum L4V4 value */
	};

	virtual std::string GetString(int nStringCode, void *pParam = 0);
	virtual std::string GetMsgTagVariable();
	virtual std::string GetInitRcvstrVariable();
	virtual std::string GetTypeName(int nType, bool bUnsigned, int nSize = 0);

};

#endif
