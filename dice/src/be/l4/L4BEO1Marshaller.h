/**
 *	\file	dice/src/be/l4/L4BEO1Marshaller.h
 *	\brief	contains the declaration of the class CL4BEO1Marshaller
 *
 *	\date	05/17/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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
#ifndef L4BEO1MARSHALLER_H
#define L4BEO1MARSHALLER_H

#include <be/BEO1Marshaller.h>

/** \class CL4BEO1Marshaller
 *  \brief the class contains the marshalling code
 */
class CL4BEO1Marshaller : public CBEO1Marshaller
{
DECLARE_DYNAMIC(CL4BEO1Marshaller);
public: 
    /** \brief constructor of marshaller */
	CL4BEO1Marshaller();
	~CL4BEO1Marshaller();

public: // Public methods
    virtual int Marshal(CBEFile * pFile, CBETypedDeclarator * pParameter, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext);
    virtual int Marshal(CBEFile * pFile, CBEFunction * pFunction, int nFEType, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext);

protected: // Protected methods
    virtual int MarshalDeclarator(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bIncOffsetVariable, bool bLastParameter, CBEContext * pContext);
    virtual int MarshalFlexpage(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext);
    virtual int MarshalIndirectString(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext);
    virtual int MarshalString(CBEType * pType, int nStartOffset, bool & bUseConstOffset, bool bLastParameter, CBEContext * pContext);

protected: // Protected attributes
    /** \var int m_nTotalFlexpages
     *  \brief the total number of flexpages in a function
     */
    int m_nTotalFlexpages;
    /** \var int m_nCurrentFlexpages
     *  \brief the current count of the marshalled flexpages
     */
    int m_nCurrentFlexpages;
    /** \var int m_nCurrentString
     *  \brief is the index into the indirect string array
     */
    int m_nCurrentString;
};

#endif
