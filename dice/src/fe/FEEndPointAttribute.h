/**
 *	\file	dice/src/fe/FEEndPointAttribute.h 
 *	\brief	contains the declaration of the class CFEEndPointAttribute
 *
 *	\date	01/31/2001
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
#ifndef __DICE_FE_FEENDPOINTATTRIBUTE_H__
#define __DICE_FE_FEENDPOINTATTRIBUTE_H__

#include "fe/FEBase.h"
#include "fe/FEAttribute.h"

/**	\class CFEPortSpec
 *	\ingroup frontend
 *	\brief a helper class for the end point attribute
 */
class CFEPortSpec : public CFEBase
{
DECLARE_DYNAMIC(CFEPortSpec);

// standard constructor/destructor
public:
	/** \brief constructs a port-spec object
	 *  \param PortStr the port spec to initialize this object
        */
	CFEPortSpec(String PortStr);
	/** a standard constructor, which initializes the members with NULL */
	CFEPortSpec();
	/** \brief constructs a port-spec object
	 *  \param FamilyString the family string of the port spec
	 *  \param PortString the port string of the port spec
	 */
	CFEPortSpec(String FamilyString, String PortString);
	virtual ~CFEPortSpec();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CFEPortSpec(CFEPortSpec &src);

// Operations
public:
	virtual void Serialize(CFile *pFile);
	virtual String GetPortString();
	virtual String GetFamiliyString();
	virtual CObject* Clone();

// attributes
protected:
	/**	\var String m_sFamily
	 *	\brief is the family string
	 */
	String m_sFamily;
	/**	\var String m_sPort
	 *	\brief is the port string
	 */
	String m_sPort;
};

/** \class CFEEndPointAttribute
 *	\ingroup frontend
 *	\brief represents the end-point attribute
 *
 * This class represents the attribute end point.
 */
class CFEEndPointAttribute : public CFEAttribute
{
DECLARE_DYNAMIC(CFEEndPointAttribute);

// standard constructor/destructor
public:
	/** \brief constructs an end-point attribute
	 *  \param pPortSpecs the ports ant the end-point of the communication
	 */
	CFEEndPointAttribute(Vector *pPortSpecs);
	virtual ~CFEEndPointAttribute();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CFEEndPointAttribute(CFEEndPointAttribute &src);

// Operations
public:
	virtual void Serialize(CFile *pFile);
	virtual CFEPortSpec* GetNextPortSpec(VectorElement* &iter);
	virtual VectorElement* GetFirstPortSpec();
	virtual CObject* Clone();

// attributes
protected:
	/**	\var Vector *m_pPortSpecs
	 *	\brief contains all port specifications
	 */
	Vector *m_pPortSpecs;
};

#endif /* __DICE_FE_FEENDPOINTATTRIBUTE_H__ */

