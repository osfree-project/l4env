/**
 *    \file    dice/src/be/cdr/CCDRMarshalFunction.cpp
 *  \brief   contains the implementation of the class CCDRMarshalFunction
 *
 *    \date    10/29/2003
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

#include "be/cdr/CCDRMarshalFunction.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEContext.h"
#include "be/BETrace.h"
#include "Attribute-Type.h"
#include <cassert>

CCDRMarshalFunction::CCDRMarshalFunction()
 : CBEMarshalFunction()
{
}

/** destroys the object */
CCDRMarshalFunction::~CCDRMarshalFunction()
{
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if this function should be written to the given file
 */
bool CCDRMarshalFunction::DoWriteFunction(CBEHeaderFile* pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
        (!m_Attributes.Find(ATTR_OUT)) &&
        !IsComponentSide())
        return true;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) &&
        (!m_Attributes.Find(ATTR_IN)) &&
        IsComponentSide())
        return true;
    return false;
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if this function should be written to the given file
 */
bool CCDRMarshalFunction::DoWriteFunction(CBEImplementationFile* pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
        (!m_Attributes.Find(ATTR_OUT)) &&
        !IsComponentSide())
        return true;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) &&
        (!m_Attributes.Find(ATTR_IN)) &&
        IsComponentSide())
        return true;
    return false;
}

/** \brief writes the marshalling code
 *  \param pFile the file to write to
 *
 * No opcode marshalled.
 */
void 
CCDRMarshalFunction::WriteMarshalling(CBEFile* pFile)
{
    assert(m_pTrace);
    bool bLocalTrace = false;
    if (!m_bTraceOn)
    {
	m_pTrace->BeforeMarshalling(pFile, this);
	m_bTraceOn = bLocalTrace = true;
    }
    
    if (IsComponentSide())
        WriteMarshalException(pFile, true);
    // now unmarshal rest
    CBEOperationFunction::WriteMarshalling(pFile);

    if (bLocalTrace)
    {
	m_pTrace->AfterMarshalling(pFile, this);
	m_bTraceOn = false;
    }
}
