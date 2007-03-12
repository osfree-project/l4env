/**
 *  \file    dice/src/be/l4/v2/L4V2BENameFactory.h
 *  \brief   contains the declaration of the class CL4V2BENameFactory
 *
 *  \date    02/05/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
 * Technische Universität Dresden, Operating Systems Research Group
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
#ifndef L4V2BENAMEFACTORY_H
#define L4V2BENAMEFACTORY_H

#include <be/l4/L4BENameFactory.h>

/** \class CL4V2BENameFactory
 *  \ingroup backend
 *  \brief contains functions to create V4 specific names
 */
class CL4V2BENameFactory : public CL4BENameFactory
{

public:
    /** \brief creates the instance of the name factory
     */
    CL4V2BENameFactory();
    virtual ~CL4V2BENameFactory();

    virtual string GetTimeoutServerVariable(CBEFunction *pFunction);
    virtual string GetTimeoutClientVariable(CBEFunction *pFunction);
};

#endif
