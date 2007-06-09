/**
 *    \file    dice/src/Error.h
 *    \brief   contains the declaration of the class CError
 *
 *    \date    11/19/2006
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006
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
#ifndef __DICE_ERROR_H__
#define __DICE_ERROR_H__

#include <string>
#include <exception>

namespace error
{
    /** \class invalid_operator
     *  \ingroup error
     *  \brief exception class for usage of invalid operator
     */
    class invalid_operator : public std::exception
    {
    public:
	/** \brief the constructor of the invalid_operator exceptions */
	explicit invalid_operator(void) throw()
	{ }

	/** \brief returns the string specification of the exceptions */
	virtual const char* what() const throw()
	{
	    return "Invalid operator used.";
	}
    };

    /** \class preprocess_error
     *  \ingroup error
     *  \brief exception class for preprocessor
     */
    class preprocess_error : public std::exception
    {
    public:
	/** \brief the constructor taking a reason */
	explicit preprocess_error(const char* str) throw()
	    : m_reason(str)
	{ }

	/** \brief required to have rigid throw specification */
	virtual ~preprocess_error() throw()
	{ }

	/** \brief returns the string of the exception */
	virtual const char* what() const throw()
	{
	    return m_reason.c_str();
	}

    private:
	/** \var string m_reason
	 *  \brief contains the reason string
	 */
	std::string m_reason;
    };

    /** \class postparse_error
     *  \ingroup error
     *  \brief exception class for preprocessing
     */
    class postparse_error : public std::exception
    {
    public:
	/** \brief the constructor for the class */
	explicit postparse_error(void) throw()
	{ }

	/** \brief returns the string of the exception */
	virtual const char* what() const throw()
	{
	    return "PostParse exception.";
	}
    };

    /** \class consistency_error
     *  \ingroup error
     *  \brief exception class for consistency check errors
     */
    class consistency_error : public std::exception
    {
    public:
	/** \brief the constructor of the class */
	explicit consistency_error(void) throw()
	{ }

	/** \brief returns the string of the exception */
	virtual const char* what() const throw()
	{
	    return "Check Consistency error.";
	}
    };

    /** \class create_error
     *  \ingroup error
     *  \brief exception class for back-end creation errors
     */
    class create_error : public std::exception
    {
	/** \var string reason
	 *  \brief the reason for the excpetion
	 */
	string reason;

    public:
	/** \brief constructs exception class
	 *  \param a detailed reason
	 */
	explicit create_error(string r) : reason(r)
	{ }

	/** \brief destructor 
	 *
	 * Because the string() constructor can throw exceptions we have to
	 * catch them here.
	 */
	~create_error() throw()
	try { } catch (...) { }

	/** \brief return the string of the exception */
	virtual const char* what() const throw()
	{
	    return reason.c_str();
	}
    };

}; /* namespace error */

#endif                // __DICE_ERROR_H__
