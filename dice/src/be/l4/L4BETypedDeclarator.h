/**
 *	\file	dice/src/be/l4/L4BETypedDeclarator.h
 *	\brief	contains the declaration of the class CL4BETypedDeclarator
 *
 *	\date	Wed Jul 17 2002
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

#ifndef L4BETYPEDDECLARATOR_H
#define L4BETYPEDDECLARATOR_H

#include <be/BETypedDeclarator.h>

/** \class CL4BETypedDeclarator
 *  \ingroup backend
 *  \brief implements L4 specific functions of the typed declarator
 */
class CL4BETypedDeclarator : public CBETypedDeclarator
{
DECLARE_DYNAMIC(CL4BETypedDeclarator);
public:
	/** creates a new object of a typed declarator */
	CL4BETypedDeclarator();
	~CL4BETypedDeclarator();

public: // Public methods
    virtual bool IsVariableSized();
    virtual bool IsFixedSized();
};

#endif
