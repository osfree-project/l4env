/**
 *  \file   dice/src/be/cdr/CCDRUnmarshalFunction.h
 *  \brief  contains the declaration of the class CCDRUnmarshalFunction
 *
 *  \date   10/29/2003
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
#ifndef CCDRUNMARSHALFUNCTION_H
#define CCDRUNMARSHALFUNCTION_H

#include <be/BEUnmarshalFunction.h>

/** \class CCDRUnmarshalFunction
 *  \ingroup backend
 *  \brief specializes functionality of the unmarshal function for CDR
 */
class CCDRUnmarshalFunction : public CBEUnmarshalFunction
{
public:
    /** creates object of this class */
    CCDRUnmarshalFunction();
    virtual ~CCDRUnmarshalFunction();

public:
    virtual bool DoWriteFunction(CBEHeaderFile* pFile);
    virtual bool DoWriteFunction(CBEImplementationFile* pFile);

protected:
    virtual void WriteUnmarshalling(CBEFile* pFile);
};

#endif
