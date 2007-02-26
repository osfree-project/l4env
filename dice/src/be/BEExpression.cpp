/**
 *	\file	dice/src/be/BEExpression.cpp
 *	\brief	contains the implementation of the class CBEExpression
 *
 *	\date	01/17/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

#include "be/BEExpression.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BERoot.h"
#include "be/BETypedef.h"

#include "fe/FEExpression.h"
#include "fe/FEUserDefinedExpression.h"
#include "fe/FEPrimaryExpression.h"
#include "fe/FEUnaryExpression.h"
#include "fe/FEBinaryExpression.h"
#include "fe/FEConditionalExpression.h"
#include "fe/FESizeOfExpression.h"
#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CBEExpression);

CBEExpression::CBEExpression()
{
    m_nType = EXPR_NONE;
    m_pCondition = 0;
    m_pOperand1 = 0;
    m_pOperand2 = 0;
	m_pType = 0;
    m_nIntValue = 0;
    m_fFloatValue = 0.0;
    m_nOperator = 0;
    IMPLEMENT_DYNAMIC_BASE(CBEExpression, CBEObject);
}

CBEExpression::CBEExpression(CBEExpression & src):CBEObject(src)
{
    m_nType = src.m_nType;
    m_nIntValue = src.m_nIntValue;
    m_fFloatValue = src.m_fFloatValue;
    m_cCharValue = src.m_cCharValue;
	if (src.m_pOperand1)
	{
	    m_pOperand1 = (CBEExpression*)src.m_pOperand1->Clone();
		m_pOperand1->SetParent(this);
	}
	else
	    m_pOperand1 = 0;
    m_sStringValue = src.m_sStringValue;
    m_nOperator = src.m_nOperator;
	if (src.m_pCondition)
	{
	    m_pCondition = (CBEExpression*)src.m_pCondition->Clone();
		m_pCondition->SetParent(this);
	}
	else
	    m_pCondition = 0;
    if (src.m_pOperand2)
	{
	    m_pOperand2 = (CBEExpression*)src.m_pOperand2->Clone();
		m_pOperand2->SetParent(this);
	}
	else
	    m_pOperand2 = 0;
	if (src.m_pType)
	{
	    m_pType = (CBEType*)src.m_pType->Clone();
		m_pType->SetParent(this);
	}
	else
	    m_pType = 0;
    IMPLEMENT_DYNAMIC_BASE(CBEExpression, CBEObject);
}

/**	\brief destructor of this instance */
CBEExpression::~CBEExpression()
{
    if (m_pOperand1)
	    delete m_pOperand1;
    if (m_pOperand2)
	    delete m_pOperand2;
    if (m_pCondition)
	    delete m_pCondition;
    if (m_pType)
	    delete m_pType;
}

/**	\brief creates the back-end representation of an expression
 *	\param pFEExpression the corresponding front-end expression
 *	\param pContext the context of the code generation
 *	\return true if code generation was successful
 */
bool CBEExpression::CreateBackEnd(CFEExpression * pFEExpression, CBEContext * pContext)
{
    m_nType = pFEExpression->GetType();
    switch (m_nType)
    {
    case EXPR_NONE:
    case EXPR_NULL:
    case EXPR_TRUE:
    case EXPR_FALSE:
        break;
    case EXPR_CHAR:
        m_cCharValue = pFEExpression->GetChar();
        break;
    case EXPR_STRING:
        m_sStringValue = pFEExpression->GetString();
        break;
    case EXPR_USER_DEFINED:
        m_sStringValue = ((CFEUserDefinedExpression *) pFEExpression)->GetExpName();
        break;
    case EXPR_INT:
        m_nIntValue = ((CFEPrimaryExpression *) pFEExpression)->GetIntValue();
        break;
    case EXPR_FLOAT:
        m_fFloatValue = ((CFEPrimaryExpression *) pFEExpression)->GetFloatValue();
        break;
    case EXPR_CONDITIONAL:
        return CreateBackEndConditional((CFEConditionalExpression *)pFEExpression, pContext);
        break;
    case EXPR_BINARY:
        return CreateBackEndBinary((CFEBinaryExpression *) pFEExpression, pContext);
        break;
    case EXPR_UNARY:
        return CreateBackEndUnary((CFEUnaryExpression *) pFEExpression, pContext);
        break;
    case EXPR_PAREN:
        return CreateBackEndPrimary((CFEPrimaryExpression *) pFEExpression, pContext);
        break;
    case EXPR_SIZEOF:
	    return CreateBackEndSizeOf((CFESizeOfExpression *) pFEExpression, pContext);
		break;
    }
    return true;
}

/**	\brief creates the back-end expression
 *	\param pFEExpression the corresponding front-end expression
 *	\param pContext the context of the code generation
 *	\return true if the code generation was successful
 */
bool CBEExpression::CreateBackEndConditional(CFEConditionalExpression *pFEExpression, CBEContext * pContext)
{
    if (!pFEExpression->GetCondition())
	{
	    VERBOSE("CBEExpression::CreateBackEndConditional failed because no condition\n");
        return false;
	}
    m_pCondition = pContext->GetClassFactory()->GetNewExpression();
    m_pCondition->SetParent(this);
    if (!m_pCondition->CreateBackEnd(pFEExpression->GetCondition(), pContext))
    {
        delete m_pCondition;
        m_pCondition = 0;
        return false;
    }
    return CreateBackEndBinary(pFEExpression, pContext);
}

/**	\brief creates the back-end expression
 *	\param pFEExpression the corresponding front-end expression
 *	\param pContext the context of the code generation
 *	\return true if the code generation was successful
 */
bool CBEExpression::CreateBackEndBinary(CFEBinaryExpression * pFEExpression, CBEContext * pContext)
{
    if (!pFEExpression->GetOperand2())
	{
	    VERBOSE("CBEExpression::CreateBackEndBinary failed because no second operand\n");
		return false;
	}
    m_pOperand2 = pContext->GetClassFactory()->GetNewExpression();
    m_pOperand2->SetParent(this);
    if (!m_pOperand2->CreateBackEnd(pFEExpression->GetOperand2(), pContext))
    {
        delete m_pOperand2;
        m_pOperand2 = 0;
        return false;
    }
    return CreateBackEndUnary(pFEExpression, pContext);
}

/**	\brief creates the back-end expression
 *	\param pFEExpression the corresponding front-end expression
 *	\param pContext the context of the code generation
 *	\return true if the code generation was successful
 */
bool CBEExpression::CreateBackEndUnary(CFEUnaryExpression * pFEExpression, CBEContext * pContext)
{
    m_nOperator = pFEExpression->GetOperator();
    return CreateBackEndPrimary(pFEExpression, pContext);
}

/** \brief creates back end unary expression
 *  \param nOperator specifies the operator fo the unary expression
 *  \param pOperand the operand
 *  \param pContext the context of this creation
 *  \return true if successful
 */
bool CBEExpression::CreateBackEndUnary(int nOperator, CBEExpression *pOperand, CBEContext * pContext)
{
    m_nOperator = nOperator;
    m_pOperand1 = pOperand;
    m_pOperand1->SetParent(this);
    return true;
}

/**	\brief creates the back-end expression
 *	\param pFEExpression the corresponding front-end expression
 *	\param pContext the context of the code generation
 *	\return true if the code generation was successful
 *
 * This function is only called to create an expression for m_pOperand1:
 */
bool CBEExpression::CreateBackEndPrimary(CFEPrimaryExpression * pFEExpression, CBEContext * pContext)
{
    CFEExpression *pFEOperand = pFEExpression->GetOperand();
	if (!pFEOperand)
	{
	    VERBOSE("CBEExpression::CreateBackEndPrimary failed because no expression\n");
		return false;
	}
    CBEExpression *pExpression = pContext->GetClassFactory()->GetNewExpression();
    pExpression->SetParent(this);
    if (!pExpression->CreateBackEnd(pFEOperand, pContext))
    {
        delete pExpression;
        return false;
    }
    m_pOperand1 = pExpression;
    return true;
}

/** \brief creates a primary expression
 *  \param nType the type of the new expression
 *  \param pExpression a source of information
 *  \param pContext the context of this create operation
 *  \return true if successful
 */
bool CBEExpression::CreateBackEndPrimary(int nType, CBEExpression *pExpression, CBEContext *pContext)
{
    m_nType = nType;
    switch (m_nType)
    {
    case EXPR_NONE:
    case EXPR_NULL:
    case EXPR_TRUE:
    case EXPR_FALSE:
        break;
    case EXPR_CHAR:
        m_cCharValue = pExpression->m_cCharValue;
        break;
    case EXPR_STRING:
    case EXPR_USER_DEFINED:
        m_sStringValue = pExpression->m_sStringValue;
        break;
    case EXPR_INT:
        m_nIntValue = pExpression->m_nIntValue;
        break;
    case EXPR_FLOAT:
        m_fFloatValue = pExpression->m_fFloatValue;
        break;
    case EXPR_CONDITIONAL:
        m_pCondition = pExpression->m_pCondition;
    case EXPR_BINARY:
        m_pOperand2 = pExpression->m_pOperand2;
    case EXPR_UNARY:
        m_pOperand1 = pExpression->m_pOperand1;
        m_nOperator = pExpression->m_nOperator;
        break;
    case EXPR_PAREN:
        m_pOperand1 = pExpression;
        break;
    }
    return true;
}

/** \brief create the size-of expression
 *  \param pExpression the size of expression
 *  \param pContext the context of the create
 *  \return true if successful
 */
bool CBEExpression::CreateBackEndSizeOf(CFESizeOfExpression *pFEExpression, CBEContext *pContext)
{
    // can be type
	CFETypeSpec *pFEType = pFEExpression->GetSizeOfType();
	if (pFEType)
	{
	    CBEType *pType = pContext->GetClassFactory()->GetNewType(pFEType->GetType());
		pType->SetParent(this);
		if (!pType->CreateBackEnd(pFEType, pContext))
		{
		    delete pType;
			VERBOSE("CBEExpression::CreateBackEndSizeOf failed because type could not be created.\n");
			return false;
        }
		m_pType = pType;
	    return true;
	}
	// can be expression
	CFEExpression *pFESizeOfExpression = pFEExpression->GetSizeOfExpression();
	if (pFESizeOfExpression)
	{
	    CBEExpression *pExpression = pContext->GetClassFactory()->GetNewExpression();
		pExpression->SetParent(this);
		if (!pExpression->CreateBackEnd(pFESizeOfExpression, pContext))
		{
		    delete pExpression;
			VERBOSE("CBEExpression::CreateBackEndSizeOf failed because expression could not be created.\n");
			return false;
		}
		m_pOperand1 = pExpression;
	    return true;
	}
	// then it is a string
    m_sStringValue = pFEExpression->GetString();
	return true;
}

/**	\brief write the content of the expression to the target file
 *	\param pFile the target file to write to
 *	\param pContext the context of the write operation
 */
void CBEExpression::Write(CBEFile * pFile, CBEContext * pContext)
{
    if (!pFile->IsOpen())
        return;

    switch (m_nType)
    {
    case EXPR_NONE:
        break;
    case EXPR_NULL:
        pFile->Print("0");
        break;
    case EXPR_TRUE:
        pFile->Print("true");
        break;
    case EXPR_FALSE:
        pFile->Print("false");
        break;
    case EXPR_CHAR:
        pFile->Print("%c", m_cCharValue);
        break;
    case EXPR_STRING:
        if (m_sStringValue.IsEmpty())
            pFile->Print("0");
        else
            pFile->Print("\"%s\"", (const char *) m_sStringValue);
        break;
    case EXPR_USER_DEFINED:
        if (m_sStringValue.IsEmpty())
            pFile->Print("0");
        else
            pFile->Print("%s", (const char *) m_sStringValue);
        break;
    case EXPR_INT:
        pFile->Print("%d", m_nIntValue);
        break;
    case EXPR_FLOAT:
        pFile->Print("%f", m_fFloatValue);
        break;
    case EXPR_CONDITIONAL:
        WriteConditional(pFile, pContext);
        break;
    case EXPR_BINARY:
        WriteBinary(pFile, pContext);
        break;
    case EXPR_UNARY:
        WriteUnary(pFile, pContext);
        break;
    case EXPR_PAREN:
        pFile->Print("( ");
        m_pOperand1->Write(pFile, pContext);
        pFile->Print(" )");
        break;
    }
}

/**	\brief writes a conditional expression to the target file
 *	\param pFile the target file to write to
 *	\param pContext the context of the write operation
 */
void CBEExpression::WriteConditional(CBEFile * pFile, CBEContext * pContext)
{
    pFile->Print("(");
    m_pCondition->Write(pFile, pContext);
    pFile->Print(") ? (");
    m_pOperand1->Write(pFile, pContext);
    pFile->Print(") : (");
    m_pOperand2->Write(pFile, pContext);
    pFile->Print(")");
}

/**	\brief writes a binary expression to the target file
 *	\param pFile the target file to write to
 *	\param pContext the context of the write operation
 */
void CBEExpression::WriteBinary(CBEFile * pFile, CBEContext * pContext)
{
    // write first operand
    m_pOperand1->Write(pFile, pContext);
    // write operator
    switch (m_nOperator)
      {
      case EXPR_MUL:
	  pFile->Print(" * ");
	  break;
      case EXPR_DIV:
	  pFile->Print(" / ");
	  break;
      case EXPR_MOD:
	  pFile->Print(" % ");
	  break;
      case EXPR_PLUS:
	  pFile->Print(" + ");
	  break;
      case EXPR_MINUS:
	  pFile->Print(" - ");
	  break;
      case EXPR_LSHIFT:
	  pFile->Print(" << ");
	  break;
      case EXPR_RSHIFT:
	  pFile->Print(" >> ");
	  break;
      case EXPR_LT:
	  pFile->Print(" < ");
	  break;
      case EXPR_GT:
	  pFile->Print(" > ");
	  break;
      case EXPR_LTEQU:
	  pFile->Print(" <= ");
	  break;
      case EXPR_GTEQU:
	  pFile->Print(" >= ");
	  break;
      case EXPR_EQUALS:
	  pFile->Print(" == ");
	  break;
      case EXPR_NOTEQUAL:
	  pFile->Print(" != ");
	  break;
      case EXPR_BITAND:
	  pFile->Print(" & ");
	  break;
      case EXPR_BITXOR:
	  pFile->Print(" ^ ");
	  break;
      case EXPR_BITOR:
	  pFile->Print(" | ");
	  break;
      case EXPR_LOGAND:
	  pFile->Print(" && ");
	  break;
      case EXPR_LOGOR:
	  pFile->Print(" || ");
	  break;
      default:
	  assert(false);
	  break;
      }
    // write second operand
    m_pOperand2->Write(pFile, pContext);
}

/**	\brief writes a unary expression to the target file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CBEExpression::WriteUnary(CBEFile * pFile, CBEContext * pContext)
{
    switch (m_nOperator)
      {
      case EXPR_SPLUS:
	  pFile->Print("+");
	  break;
      case EXPR_SMINUS:
	  pFile->Print("-");
	  break;
      case EXPR_TILDE:
	  pFile->Print("~");
	  break;
      case EXPR_EXCLAM:
	  pFile->Print("!");
	  break;
      default:
	  assert(false);
	  break;
      }
    // write operand
    m_pOperand1->Write(pFile, pContext);
}

/**	\brief creates expression with integer value
 *	\param nValue the value of the expression
 *	\param pContext the context of the generation
 *	\return true if successful
 */
bool CBEExpression::CreateBackEnd(int nValue, CBEContext * pContext)
{
    m_nType = EXPR_INT;
    m_nIntValue = nValue;
    return true;
}

/**	\brief creates expression with string value
 *	\param sValue the string value
 *	\param pContext the context of the generation
 *	\return true if successful
 */
bool CBEExpression::CreateBackEnd(String sValue, CBEContext * pContext)
{
    m_nType = EXPR_USER_DEFINED;
    m_sStringValue = sValue;
    return true;
}

/**	\brief creates binary expression from existing expressions
 *	\param pOperand1 the first operand
 *	\param nOperator the operator
 *	\param pOperand2 the second operand
 *	\param pContext the context of the creation
 *	\return true if successful
 */
bool CBEExpression::CreateBackEndBinary(CBEExpression * pOperand1, int nOperator, CBEExpression * pOperand2, CBEContext * pContext)
{
    m_pOperand1 = pOperand1;
    m_nOperator = nOperator;
    m_pOperand2 = pOperand2;
    m_nType = EXPR_BINARY;
    return true;
}

/**	\brief calculates the integer value of this expression
 *	\return the integer value of this expression (or 0 if not evaluable)
 */
int CBEExpression::GetIntValue()
{
    int nValue = 0;

    switch (m_nType)
	{
	case EXPR_NONE:
	case EXPR_NULL:
	case EXPR_STRING:
	case EXPR_USER_DEFINED:
		break;
	case EXPR_TRUE:
		nValue = (int) true;
		break;
	case EXPR_FALSE:
		nValue = (int) false;
		break;
	case EXPR_CHAR:
		nValue = (int) m_cCharValue;
		break;
	case EXPR_INT:
		nValue = m_nIntValue;
		break;
	case EXPR_FLOAT:
		nValue = (int) m_fFloatValue;
		break;
	case EXPR_CONDITIONAL:
		if (m_pCondition->GetBoolValue() == true)
			nValue = m_pOperand1->GetIntValue();
		else
			nValue = m_pOperand2->GetIntValue();
		break;
	case EXPR_BINARY:
		nValue = GetIntValueBinary();
		break;
	case EXPR_UNARY:
		nValue = GetIntValueUnary();
		break;
	case EXPR_PAREN:
		nValue = m_pOperand1->GetIntValue();
		break;
	case EXPR_SIZEOF:
	    if (m_pOperand1)
		    nValue = m_pOperand1->GetIntValue(); // cause that's what sizeof does
	    else if (m_pType)
		    nValue = m_pType->GetSize(); // sizeof(type)
	    else
		{
		    // check for user defined type
			CBERoot *pRoot = GetRoot();
			assert(pRoot);
			CBETypedef *pUserDefined = pRoot->FindTypedef(m_sStringValue);
			assert(pUserDefined);
			nValue = pUserDefined->GetSize();
		}
		break;
	}

    return nValue;
}

/**	\brief calculates the integer value for a binary expression
 *	\return the caculated integer expression
 */
int CBEExpression::GetIntValueBinary()
{
    int nValue1 = m_pOperand1->GetIntValue();
    int nValue2 = m_pOperand2->GetIntValue();
    int nValue = 0;

    switch (m_nOperator)
	{
	case EXPR_MUL:
		nValue = nValue1 * nValue2;
		break;
	case EXPR_DIV:
		if (nValue2)
			nValue = nValue1 / nValue2;
		break;
	case EXPR_MOD:
		if (nValue2)
			nValue = nValue1 % nValue2;
		break;
	case EXPR_PLUS:
		nValue = nValue1 + nValue2;
		break;
	case EXPR_MINUS:
		nValue = nValue1 - nValue2;
		break;
	case EXPR_LSHIFT:
		nValue = nValue1 << nValue2;
		break;
	case EXPR_RSHIFT:
		nValue = nValue1 >> nValue2;
		break;
	case EXPR_LT:
		nValue = (int) (nValue1 < nValue2);
		break;
	case EXPR_GT:
		nValue = (int) (nValue1 > nValue2);
		break;
	case EXPR_LTEQU:
		nValue = (int) (nValue1 <= nValue2);
		break;
	case EXPR_GTEQU:
		nValue = (int) (nValue1 >= nValue2);
		break;
	case EXPR_EQUALS:
		nValue = (int) (nValue1 == nValue2);
		break;
	case EXPR_NOTEQUAL:
		nValue = (int) (nValue1 != nValue2);
		break;
	case EXPR_BITAND:
		nValue = nValue1 & nValue2;
		break;
	case EXPR_BITXOR:
		nValue = nValue1 ^ nValue2;
		break;
	case EXPR_BITOR:
		nValue = nValue1 | nValue2;
		break;
	case EXPR_LOGAND:
		nValue = (int) (nValue1 && nValue2);
		break;
	case EXPR_LOGOR:
		nValue = (int) (nValue1 || nValue2);
		break;
	default:
		assert(false);
		break;
	}

    return nValue;
}

/**	\brief calculates the unary expression
 *	\return the calculated value
 */
int CBEExpression::GetIntValueUnary()
{
    int nValue = m_pOperand1->GetIntValue();

    switch (m_nOperator)
	{
	case EXPR_SPLUS:
		nValue = +nValue;
		break;
	case EXPR_SMINUS:
		nValue = -nValue;
		break;
	case EXPR_TILDE:
		nValue = ~nValue;
		break;
	case EXPR_EXCLAM:
		nValue = !nValue;
		break;
	default:
		assert(false);
		break;
	}

    return nValue;
}

/**	\brief evaluates the expression to a boolean value
 *	\return true or false
 */
bool CBEExpression::GetBoolValue()
{
    bool bValue = false;

    switch (m_nType)
	{
	case EXPR_NONE:
	case EXPR_NULL:
	case EXPR_STRING:
	case EXPR_USER_DEFINED:
	case EXPR_SIZEOF:
		break;
	case EXPR_TRUE:
		bValue = true;
		break;
	case EXPR_FALSE:
		bValue = false;
		break;
	case EXPR_CHAR:
		bValue = (bool) m_cCharValue;
		break;
	case EXPR_INT:
		bValue = (bool) m_nIntValue;
		break;
	case EXPR_FLOAT:
		bValue = (bool) m_fFloatValue;
		break;
	case EXPR_CONDITIONAL:
		if (m_pCondition->GetBoolValue() == true)
			bValue = m_pOperand1->GetBoolValue();
		else
			bValue = m_pOperand2->GetBoolValue();
		break;
	case EXPR_BINARY:
		bValue = GetBoolValueBinary();
		break;
	case EXPR_UNARY:
		bValue = GetBoolValueUnary();
		break;
	case EXPR_PAREN:
		bValue = m_pOperand1->GetBoolValue();
		break;
	}

    return bValue;
}

/**	\brief calculates the boolean value for the binary expression
 *	\return true or false
 */
bool CBEExpression::GetBoolValueBinary()
{
    bool bValue1 = m_pOperand1->GetBoolValue();
    bool bValue2 = m_pOperand2->GetBoolValue();
    bool bValue = false;

    switch (m_nOperator)
      {
      case EXPR_MUL:
	  bValue = (bool) ((int) bValue1 * (int) bValue2);
	  break;
      case EXPR_DIV:
	  if ((int) bValue2)
	      bValue = (bool) ((int) bValue1 / (int) bValue2);
	  break;
      case EXPR_MOD:
	  if ((int) bValue2)
	      bValue = (bool) ((int) bValue1 % (int) bValue2);
	  break;
      case EXPR_PLUS:
	  bValue = (bool) ((int) bValue1 + (int) bValue2);
	  break;
      case EXPR_MINUS:
	  bValue = (bool) ((int) bValue1 - (int) bValue2);
	  break;
      case EXPR_LSHIFT:
	  bValue = (bool) ((int) bValue1 << (int) bValue2);
	  break;
      case EXPR_RSHIFT:
	  bValue = (bool) ((int) bValue1 >> (int) bValue2);
	  break;
      case EXPR_LT:
	  bValue = bValue1 < bValue2;
	  break;
      case EXPR_GT:
	  bValue = bValue1 > bValue2;
	  break;
      case EXPR_LTEQU:
	  bValue = bValue1 <= bValue2;
	  break;
      case EXPR_GTEQU:
	  bValue = bValue1 >= bValue2;
	  break;
      case EXPR_EQUALS:
	  bValue = bValue1 == bValue2;
	  break;
      case EXPR_NOTEQUAL:
	  bValue = bValue1 != bValue2;
	  break;
      case EXPR_BITAND:
	  bValue = bValue1 & bValue2;
	  break;
      case EXPR_BITXOR:
	  bValue = bValue1 ^ bValue2;
	  break;
      case EXPR_BITOR:
	  bValue = bValue1 | bValue2;
	  break;
      case EXPR_LOGAND:
	  bValue = bValue1 && bValue2;
	  break;
      case EXPR_LOGOR:
	  bValue = bValue1 || bValue2;
	  break;
      default:
	  assert(false);
	  break;
      }

    return bValue;
}

/**	\brief calculates the boolean value for the unary expression
 *	\return true or false
 */
bool CBEExpression::GetBoolValueUnary()
{
    bool bValue = m_pOperand1->GetBoolValue();

    switch (m_nOperator)
      {
      case EXPR_SPLUS:
	  bValue = (bool) (+((int) bValue));
	  break;
      case EXPR_SMINUS:
	  bValue = (bool) (-((int) bValue));
	  break;
      case EXPR_TILDE:
	  {
	      int iValue = (int) bValue;
	      iValue = ~iValue;
	      bValue = (bool) iValue;
	  }
	  break;
      case EXPR_EXCLAM:
	  bValue = !bValue;
	  break;
      default:
	  assert(false);
	  break;
      }

    return bValue;
}

/** \brief tests for a specific front-end type
 *  \param nFEType the front-end type to test for
 *  \return true if this is of the given FE type
 */
bool CBEExpression::IsOfType(int nFEType)
{
    return (m_nType == nFEType);
}

/** \brief creates a new instance of the expression object
 *  \return a reference to the new instance
 */
CObject* CBEExpression::Clone()
{
    return new CBEExpression(*this);
}

