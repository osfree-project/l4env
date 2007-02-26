/**
 *	\file	dice/src/be/BEEnumType.h
 *	\brief	contains the declaration of the class CBEEnumType
 *
 *	\date	Tue Jul 23 2002
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

#ifndef BEENUMTYPE_H
#define BEENUMTYPE_H

#include <be/BEType.h>

class String;

/** \class CBEEnumType
 *  \ingroup backend
 *  \brief is the type of an enum declarator
 */
class CBEEnumType : public CBEType
{
DECLARE_DYNAMIC(CBEEnumType);
public:
	/** \brief constructs an enum type class */
	CBEEnumType();
	~CBEEnumType();

public: // Public methods
    virtual int GetMemberCount();
	virtual String GetMemberAt(int nIndex);
	virtual void RemoveMember(String sMember);
	virtual void AddMember(String sMember);
	virtual bool CreateBackEnd(CFETypeSpec *pFEType, CBEContext *pContext);
	virtual void WriteZeroInit(CBEFile * pFile, CBEContext * pContext);
	virtual void Write(CBEFile * pFile, CBEContext * pContext);
    virtual bool HasTag(String sTag);

protected: // Protected attributes
    /** \var String **m_pMembers
     *  \brief contains the members of the enum type
     */
    String **m_pMembers;
    /** \var int m_nMemberCount;
     *  \brief contains the number of members
     */
    int m_nMemberCount;
	/**	\var String m_sTag
	 *	\brief the tag if the source is a tagged struct
	 */
	String m_sTag;
};

#endif
