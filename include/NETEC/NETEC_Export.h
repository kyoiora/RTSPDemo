//NETEC_Export.h

#ifndef __NETEC_EXPORT_H__
#define __NETEC_EXPORT_H__

#ifdef NETEC_EXPORT
#define NETEC_API _declspec(dllexport)
#elif NETEC_DLL
#define NETEC_API _declspec(dllimport)
#else
#define NETEC_API
#endif

#endif