/**
 *  \file   dice/src/be/l4/L4BESwitchCase.h
 *  \brief  contains the declaration of the class CL4BESwitchCase
 *
 *  \date   12/12/2003
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#ifndef L4BESWITCHCASE_H
#define L4BESWITCHCASE_H

#include <be/BESwitchCase.h>

/** \class CL4BESwitchCase
 *  \ingroup backend
 *  \brief the switch case class for the L4 back-end
 */
class CL4BESwitchCase : public CBESwitchCase
{
public:
    /** \brief creates this class */
    CL4BESwitchCase();
    ~CL4BESwitchCase();


protected:
    /** \brief copy constructor
     *  \param src the original
     */
    CL4BESwitchCase(CL4BESwitchCase &src);

protected:
    virtual void WriteVariableInitialization(CBEFile& pFile, DIRECTION_TYPE nDirection);
    virtual void WriteCleanup(CBEFile& pFile);
};

#endif
