// XAutoLock.h: interface for the AutoLock class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __XAUTOLOCK_H__
#define __XAUTOLOCK_H__

#include "NETEC/NETEC_Export.h"

#include "NETEC/XCritSec.h"

class NETEC_API XAutoLock  
{
public:
	XAutoLock(XCritSec&rXCritSec);
	~XAutoLock(void);
protected:
    XCritSec& m_rXCritSec;
};

#endif 