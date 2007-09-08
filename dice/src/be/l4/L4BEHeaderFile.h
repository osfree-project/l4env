/**
 *  \file   dice/src/be/l4/L4BEHeaderFile.h
 *  \brief  contains the declaration of the class CL4BEHeaderFile
 *
 *  \date   03/25/2002
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_L4BEHEADERFILE_H__
#define __DICE_L4BEHEADERFILE_H__

#include "be/BEHeaderFile.h"

/** \class CL4BEHeaderFile
 *  \ingroup backend
 *  \brief the header file class
 */
class CL4BEHeaderFile : public CBEHeaderFile
{
// Constructor
public:
    /** \brief constructor
     */
    CL4BEHeaderFile();
    ~CL4BEHeaderFile();

protected:
    virtual void WriteHelperFunctions();
};

#endif // !__DICE_L4BEHEADERFILE_H__
