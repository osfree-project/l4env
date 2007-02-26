/**
 *  \file   dice/src/be/cdr/CCDRMarshalFunction.h
 *  \brief  contains the declaration of the class CCDRMarshalFunction
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
#ifndef CCDRMARSHALFUNCTION_H
#define CCDRMARSHALFUNCTION_H

#include <be/BEMarshalFunction.h>

/** \class CCDRMarshalFunction
 *  \ingroup backend
 *  \brief specializes functionality of marshal function for CDR
 */
class CCDRMarshalFunction : public CBEMarshalFunction
{
public:
    /** creates object */
    CCDRMarshalFunction();
    virtual ~CCDRMarshalFunction();

public:
    virtual bool DoWriteFunction(CBEHeaderFile* pFile,  CBEContext* pContext);
    virtual bool DoWriteFunction(CBEImplementationFile* pFile,  CBEContext* pContext);

protected:
    virtual void WriteMarshalling(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext);
};

#endif
