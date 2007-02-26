
#ifndef L4X0BESIZES_H
#define L4X0BESIZES_H


#include "be/l4/L4BESizes.h"

/** \class CL4X0BESizes
 *  \brief implementes L4 X0 specific sizes
 * 
 * Ronald Aigner
 **/
class CL4X0BESizes : public CL4BESizes
{
DECLARE_DYNAMIC(CL4X0BESizes);

public:
  CL4X0BESizes();
  ~CL4X0BESizes();

public: // Public methods
    virtual int GetSizeOfEnvType(String sName);
    virtual int GetMaxShortIPCSize(int nDirection);
};

#endif
