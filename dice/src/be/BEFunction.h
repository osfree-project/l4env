/**
 *  \file    dice/src/be/BEFunction.h
 *  \brief   contains the declaration of the class CBEFunction
 *
 *  \date    01/11/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#ifndef __DICE_BEFUNCTION_H__
#define __DICE_BEFUNCTION_H__

#include "BEObject.h"
#include "BEContext.h" // FUNCTION_TYPE
#include "BEAttribute.h"
#include "BEDeclarator.h"
#include "Attribute-Type.h"
#include "template.h"

class CBETypedDeclarator;
class CBEType;
class CBEFile;
class CBEHeaderFile;
class CBEImplementationFile;
class CBEContext;
class CBEDeclarator;
class CBEException;
class CBEOpcodeType;
class CBEReplyCodeType;
class CBEClass;
class CBETarget;
class CBECommunication;
class CBETrace;
class CBEMarshaller;
class CBEMsgBuffer;

class CFEInterface;
class CFEOperation;
class CFETypeSpec;

/** \enum DIRECTION_TYPE
 *  \brief the directions possible
 */
enum DIRECTION_TYPE {
    DIRECTION_NONE,
    DIRECTION_IN,
    DIRECTION_OUT,
    DIRECTION_INOUT
};

/** \class CBEFunction
 *  \ingroup backend
 *  \brief the function class for the back-end
 *
 * This class contains the relevant informations about the target functions.
 * such as name, return type, attributes, parameters, exceptions, etc.
 */
class CBEFunction : public CBEObject
{
// Constructor
public:
    /**  \brief constructor
     */
    CBEFunction(FUNCTION_TYPE nFunctionType);
    virtual ~ CBEFunction();

protected:
    /**  \brief copy constructor */
    CBEFunction(CBEFunction & src);

public:
    /** \brief tests if this function should be written
     *  \param pFile the target file this function should be added to
     *  \return true if successful
     */
    virtual bool DoWriteFunction(CBEHeaderFile *pFile) = 0;
    /** \brief tests if this function should be written
     *  \param pFile the target file this function should be added to
     *  \return true if successful
     */
    virtual bool DoWriteFunction(CBEImplementationFile *pFile) = 0;
    
    virtual int GetSize(DIRECTION_TYPE nDirection);
    virtual int GetMaxSize(DIRECTION_TYPE nDirection);
    virtual int GetVariableSizedParameterCount(DIRECTION_TYPE nDirection);
    virtual int GetFixedSize(DIRECTION_TYPE nDirection);
    virtual int GetStringParameterCount(DIRECTION_TYPE nDirection, 
	    ATTR_TYPE nMustAttrs = ATTR_NONE, ATTR_TYPE nMustNotAttrs = ATTR_NONE);
    virtual void WriteCall(CBEFile * pFile, string sReturnVar,
	bool bCallFromSameClass);
    virtual void Write(CBEImplementationFile * pFile);
    virtual void Write(CBEHeaderFile * pFile);
    
    virtual CBETypedDeclarator* FindParameter(string sName, bool bCall = false);
    virtual CBETypedDeclarator* FindParameter(CDeclStack* pStack, bool bCall = false);
    virtual CBETypedDeclarator* FindParameterType(string sTypeName);
    virtual CBETypedDeclarator* FindParameterAttribute(ATTR_TYPE nAttributeType);
    virtual CBETypedDeclarator* FindParameterIsAttribute(ATTR_TYPE nAttributeType, 
	    string sAttributeParameter);

    virtual CBEType *GetReturnType(void);
    virtual bool DoMarshalParameter(CBETypedDeclarator *pParameter,
	    bool bMarshal);
    virtual bool HasAdditionalReference(CBEDeclarator *pDeclarator, 
	bool bCall = false);
    virtual int GetParameterCount(int nFEType, DIRECTION_TYPE nDirection);
    virtual int GetParameterCount(ATTR_TYPE nMustAttrs, ATTR_TYPE nMustNotAttrs, 
	    DIRECTION_TYPE nDirection);
    virtual void SetMsgBufferCastOnCall(bool bCastMsgBufferOnCall);
    virtual void AddToHeader(CBEHeaderFile *pHeader);
    virtual void AddToImpl(CBEImplementationFile *pImpl);
    bool IsComponentSide();
    virtual void SetComponentSide(bool bComponentSide);
    virtual bool HasVariableSizedParameters(DIRECTION_TYPE nDirection);
    virtual bool HasArrayParameters(DIRECTION_TYPE nDirection);
    virtual DIRECTION_TYPE GetReceiveDirection();
    virtual DIRECTION_TYPE GetSendDirection();
    virtual void SetCallVariable(string sOriginalName, int nStars, 
	    string sCallName);
    virtual void RemoveCallVariable(string sCallName);
    virtual string GetOpcodeConstName();
    virtual void WriteReturn(CBEFile * pFile);
    virtual int GetParameterAlignment(int nCurrentOffset, int nParamSize);

    virtual void AddLocalVariable(string sUserType,
	string sName, int nStars, string sInit = string());
    virtual void AddLocalVariable(int nFEType, bool bUnsigned, int nSize, 
	string sName, int nStars, 
	string sInit = string());
    virtual bool MsgBufferInitialization(CBEMsgBuffer * pMsgBuffer);

    virtual CBETypedDeclarator* GetReturnVariable(void);
    virtual CBETypedDeclarator* GetExceptionVariable(void);

    CBEMsgBuffer* GetMessageBuffer() const;
    CBECommunication* GetCommunication() const;
    CBEMarshaller* GetMarshaller() const;
    CBETypedDeclarator* GetObject() const;
    CBETypedDeclarator* GetEnvironment() const;
    CBETrace* GetTrace() const;
    string GetName() const;
    void SetFunctionName(CFEOperation *pFEOperation, FUNCTION_TYPE nFunctionType);
    void SetFunctionName(CFEInterface *pFEInterface, FUNCTION_TYPE nFunctionType);
    void SetFunctionName(string sName, string sOriginalName = string());
    string GetOriginalName() const;

    /** \brief test if function type matches
     *  \param nFunctionType the type to test for
     */
    bool IsFunctionType(FUNCTION_TYPE nFunctionType)
    { return m_nFunctionType == nFunctionType; }

protected:
    /** \brief writes the initialization of the variables
     *  \param pFile the file to write to
     */
    virtual void WriteVariableInitialization(CBEFile * pFile) = 0;
    /** \brief writes the invocation of the message transfer
     *  \param pFile the file to write to
     */
    virtual void WriteInvocation(CBEFile * pFile) = 0;

    virtual void WriteReturnType(CBEFile * pFile);
    virtual void WriteFunctionDefinition(CBEFile* pFile);
    virtual void WriteFunctionDeclaration(CBEFile* pFile);
    virtual bool DoWriteFunctionInline(CBEFile *pFile);
    virtual void WriteCleanup(CBEFile * pFile);
    virtual void WriteVariableDeclaration(CBEFile* pFile);
    virtual void WriteBody(CBEFile * pFile);
    virtual void WriteUnmarshalling(CBEFile * pFile);
    virtual void WriteMarshalling(CBEFile * pFile);
    virtual int WriteMarshalReturn(CBEFile * pFile, bool bMarshal);
    virtual int WriteMarshalOpcode(CBEFile *pFile, bool bMarshal);
    virtual void WriteMarshalException(CBEFile* pFile, bool bMarshal, bool bReturn);
    virtual void WriteParameterName(CBEFile * pFile, 
	    CBEDeclarator * pDeclarator);
    virtual void WriteParameter(CBEFile * pFile, 
	    CBETypedDeclarator * pParameter, 
	    bool bUseConst = true);
    virtual bool DoWriteParameter(CBETypedDeclarator *pParam);
    virtual bool WriteParameterList(CBEFile * pFile);
    virtual void WriteCallParameterName(CBEFile * pFile, 
	    CBEDeclarator * pInternalDecl, CBEDeclarator * pExternalDecl);
    virtual void WriteCallParameter(CBEFile * pFile, 
	    CBETypedDeclarator * pParameter, bool bCallFromSameClass);
    virtual void WriteCallParameterList(CBEFile * pFile, bool bCallFromSameClass);
    virtual void WriteFunctionAttributes(CBEFile* pFile);
    void AddMessageBuffer(void);
    void AddMessageBuffer(CFEOperation *pFEOperation);
    virtual bool SetReturnVar(bool bUnsigned, int nSize, int nFEType, 
	    string sName);
    virtual bool SetReturnVar(CBEType * pType, string sName);
    virtual bool SetReturnVar(CFETypeSpec * pFEType, string sName);
    virtual void SetReturnVarAttributes(CBETypedDeclarator *pReturn);
    virtual void CreateBackEnd(CFEBase* pFEObject);
    virtual void CreateObject(void);
    virtual void CreateEnvironment(void);
    void CreateMarshaller(void);
    void CreateCommunication(void);
    void CreateTrace(void);
    virtual void AddExceptionVariable();

    virtual int GetFixedReturnSize(DIRECTION_TYPE nDirection);
    virtual int GetReturnSize(DIRECTION_TYPE nDirection);
    virtual int GetMaxReturnSize(DIRECTION_TYPE nDirection);
    virtual CBETypedDeclarator* GetParameter(CBEDeclarator *pDeclarator, 
	    bool bCall);
    virtual void WriteExceptionWordInitialization(CBEFile* pFile);
    virtual void WriteExceptionCheck(CBEFile *pFile);
    virtual string GetExceptionWordInitString();

    virtual void AddAfterParameters(void);
    virtual void AddBeforeParameters(void);
    void AddLocalVariable(CBETypedDeclarator *pVariable);

    CBETypedDeclarator* FindParameterMember(CBETypedDeclarator *pParameter,
	CDeclStack::iterator iter, CDeclStack* pStack);

    CBETypedDeclarator* CreateOpcodeVariable(void);
    
private:
    void SetCallVariable(CBETypedDeclarator *pTypedDecl, 
	    string sNewDeclName, int nStars);
    
protected:
    /**  \var bool m_bComponentSide
    *   \brief true if this function is at the client side
    */
    bool m_bComponentSide;
    /**   \var bool m_bCastMsgBufferOnCall
    *  \brief true if the message buffer should be cast to local type on call
    */
    bool m_bCastMsgBufferOnCall;
    /** \var int m_nParameterIndent
    *  \brief the indent of the function's parameters
    */
    int m_nParameterIndent;
    /** \var string m_sOpcodeConstName
    *  \brief the name of the functions opcode
    */
    string m_sOpcodeConstName;
    /** \var CBEClass *m_pClass
    *  \brief a reference to the class of this function
    */
    CBEClass* m_pClass;
    /** \var CBETarget *m_pTarget
    *  \brief a reference to the corresponding target class
    */
    CBETarget *m_pTarget;
    /** \var string m_sErrorFunction
    *  \brief contains the name of the error function
    */
    string m_sErrorFunction;
    /** \var CBETrace* m_pTrace
     *  \brief a reference to the tracing class
     */
    CBETrace *m_pTrace;
    /** \var bool m_bTraceOn
     *  \brief true if tracing code has been written
     *
     * Because methods can be overloaded and derived methods call base
     * methods, it is necessary to indicate whether a method already does
     * tracing in a derived method. This way we avoid double invocation of
     * e.g. CBETrace::BeforeMarshalling.
     */
    bool m_bTraceOn;

public:
    /** \var CSearchableCollection<CBEAttribute, ATTR_TYPE> m_Attributes
     *  \brief contains the attributes of the function (CBEAttribute)
     */
    CSearchableCollection<CBEAttribute, ATTR_TYPE> m_Attributes;
    /** \var CCollection<CBEException> m_Exceptions
    *  \brief contains the exceptions the function may throw
    */
    CCollection<CBEException> m_Exceptions;
    /** \var CSearchableCollection<CBETypedDeclarator, string> m_Parameters
     *  \brief contains the parameters of the functuin (CBETypedDeclarator)
     */
    CSearchableCollection<CBETypedDeclarator, string> m_Parameters;
    /** \var CSearchableCollection<CBETypedDeclarator, string> m_CallParameters
    *  \brief a copy of the parameters of the function, which contains the \
    *         names of the variables used to call this function
    */
    CSearchableCollection<CBETypedDeclarator, string> m_CallParameters;
    /** \var CSearchableCollection<CBETypedDeclarator, string> m_LocalVariables
     *  \brief contains the variables used in this function
     */
    CSearchableCollection<CBETypedDeclarator, string> m_LocalVariables;

private:
    /** \var FUNCTION_TYPE m_nFunctionType
     *  \brief the type of the function (call, unmarshal, ...)
     */
    FUNCTION_TYPE m_nFunctionType;
    /** \var CBEMarshaller *m_pMarshaller
     *  \brief reference to the marshaller class responsible for this function
     *
     * The marshaller functionality should be unique for the whole backend
     * as well. But because the marshaller class can be overloaded, it cannot
     * be static, which in turn requires to keep an instance of the concrete
     * class somewhere. Thus, we have a reference in each communication class.
     */
    CBEMarshaller *m_pMarshaller;
    /** \var CBECommunication *m_pComm
     *  \brief a reference to the communication class of this function
     *
     * Even though there should be one communication class for a backend
     * and thus be stored in the context, we have one for each function.
     */
    CBECommunication *m_pComm;
    /** \var CBETypedDeclarator *m_pCorbaObject
     *  \brief contains a reference to the CORBA Object parameter
     */
    CBETypedDeclarator *m_pCorbaObject;
    /** \var CBETypedDeclarator *m_pCorbaEnv
     *  \brief contains a reference to the CORBA Environment parameter
     */
    CBETypedDeclarator *m_pCorbaEnv;
    /** \var CBEMsgBuffer *m_pMsgBuffer
     *  \brief references the message buffer of this function
     */
    CBEMsgBuffer *m_pMsgBuffer;
    /** \var string m_sName
     *  \brief the name of the function
     *
     * This is the name of the function as it appears in the target code.
     */
    string m_sName;
    /** \var string m_sOriginalName
     *  \brief the name of the IDL specified function
     */
    string m_sOriginalName;
};

/** \brief access the corba-Object member
 *  \return a reference to the object member
 */
inline CBETypedDeclarator* 
CBEFunction::GetObject() const
{
    return m_pCorbaObject;
}

/** \brief access the corba-environment member
 *  \return a reference to the environment member
 */
inline CBETypedDeclarator* 
CBEFunction::GetEnvironment() const
{
    return m_pCorbaEnv;
}

/** \brief access the message buffer member
 *  \return a reference to the message buffer
 */
inline CBEMsgBuffer*
CBEFunction::GetMessageBuffer() const
{
    return m_pMsgBuffer; 
}

/** \brief access the communication member
 *  \return a reference to the communication member
 */
inline CBECommunication* 
CBEFunction::GetCommunication() const
{ 
    return m_pComm; 
}

/** \brief access the marshaller member
 *  \return reference to the corresponding marshaller
 */
inline CBEMarshaller* 
CBEFunction::GetMarshaller() const
{ 
    return m_pMarshaller; 
}

/** \brief access the trace member
 *  \return reference to the corresponding trace
 */
inline CBETrace* 
CBEFunction::GetTrace() const
{ 
    return m_pTrace; 
}

/** \brief retrieves the name of the function
 *  \return a reference to the name
 *
 * The returned pointer should not be manipulated.
 */
inline string 
CBEFunction::GetName() const
{
    return m_sName;
}

/** \brief retrieves the original name of the function
 *  \return a reference to the name of the IDL function
 */
inline string 
CBEFunction::GetOriginalName() const
{
    return m_sOriginalName;
}

#endif                // !__DICE_BEFUNCTION_H__
