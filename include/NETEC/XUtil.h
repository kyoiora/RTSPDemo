// XUtil.h: interface for the XUtil class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __XUTIL_H__
#define __XUTIL_H__

#include "NETEC/NETEC_Export.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string>

NETEC_API unsigned long XGetTimestamp(void);
NETEC_API unsigned short XGetLastSequence(unsigned short usSequence1,unsigned short usSequence2);
NETEC_API unsigned long XGenerateSSRC(void);
NETEC_API void XSleep(unsigned long ulMS);

#endif 