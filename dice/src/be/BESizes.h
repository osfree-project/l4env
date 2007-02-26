/**
 *    \file    dice/src/be/BESizes.h
 *    \brief   contains the declaration of the class CBESizes
 *
 *    \date    Wed Oct 9 2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#ifndef BESIZES_H
#define BESIZES_H

#include <be/BEObject.h>

/** \class CBESizes
 *  \ingroup backend
 *  \brief contains target platform specific sizes
 */
class CBESizes : public CBEObject
{
public:
    /** \brief constructor for class CBESizes */
    CBESizes();
    virtual ~CBESizes();

public:  // Public methods
    virtual int GetSizeOfType(int nFEType, int nFESize = 0);
    virtual int GetMaxSizeOfType(int nFEType);
    virtual int GetOpcodeSize();
    virtual int GetExceptionSize();
    virtual void SetOpcodeSize(int nSize);

    int WordRoundUp(int nSize);
    string WordRoundUpStr(string const &value);
    int WordsFromBytes(int nSize);
    string WordsFromBytesStr(string const &value);

protected: // Protected attributes
    /** \var int m_nOpcodeSize
     *  \brief contains the size of the opcode type in bytes
     */
    int m_nOpcodeSize;
};

#endif
