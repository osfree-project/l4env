/**
 *	\file	dice/src/be/cdr/CCDRBEClient.h
 *	\brief	contains the declaration of the class CCDRBEClient
 *
 *	\date	10/28/2003
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
#ifndef CCDRCLIENT_H
#define CCDRCLIENT_H

#include <be/BEClient.h>

/** \class CCDRBEClient
 *  \ingroup backend
 *  \brief specializes functionality of the BE client class for CDR
 */
class CCDRClient : public CBEClient
{
DECLARE_DYNAMIC(CCDRClient);
public:
	/**	\brief constructor */
    CCDRClient();
    virtual ~CCDRClient();

public:
    virtual bool CreateBackEndFunction(CFEOperation* pFEOperation,  CBEContext* pContext);
};

#endif
