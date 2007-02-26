/**
 *    \file    dice/src/Exception.h
 *    \brief   contains the declaration of the class CException
 *
 *    \date    06/30/2005
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2005
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
#ifndef __DICE_EXCEPTION_H__
#define __DICE_EXCEPTION_H__

#include <string>
using std::string;

/** \class CException
 *  \ingroup backend
 *  \brief base class for all exceptions
 */
class CException
{
// Constructor
public:
    /** the constructor for this class */
    CException();
    /** constructir with reason */
    CException(string reason);
    virtual ~CException();

    virtual void Print();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CException(CException &src);

protected:
    /** \var string m_sReason
     *  \brief contains the reason for the exception
     */
    string m_sReason;

};

#endif                // __DICE_EXCEPTION_H__
