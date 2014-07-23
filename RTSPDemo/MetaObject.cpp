#ifdef WIN32
#ifndef	WIN32_LEAN_AND_MEAN
#define	WIN32_LEAN_AND_MEAN
#endif
#define _CRT_SECURE_NO_WARNINGS
#endif			//WIN32

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>

#include "MetaObject.h"
#include "Logger.h"

RZGlobalInit RZGlobalInit::m_gi;

RZGlobalInit::RZGlobalInit()
{
	srand(static_cast<unsigned long>(time(NULL)));		//以当前时间作为随机数的种子
	// ...
}

std::vector<std::string> RZStream::StreamSplit(const std::string& _str, const char* _delim)
{
	std::vector<std::string> vAtomList;
	if (_str == "")		//字符串为空
		return vAtomList;
	if (_delim == NULL)		//分隔符为空
	{
		vAtomList.push_back(_str);
		return vAtomList;
	}

	std::string strStream = _str;
	int nLength = ::strlen(_delim);
	int nPos = 0;
	while ((nPos = strStream.find(_delim)) != -1) 
	{
		//在流中找到这个分隔符，位置可能为0
		if (nPos == 0) 
		{
			strStream.erase(0, nLength);		//删除这个分隔符
			continue;
		}
		//不在位置0，那么说明在下一个分隔符之前有一个子串，存入容器
		vAtomList.push_back(strStream.substr(0, nPos));
		strStream.erase(0, nPos+nLength);		//将子串连同分隔符一起去掉
	}
	if (strStream.length() != 0)		//若还有剩余子串
		vAtomList.push_back(strStream);
	return vAtomList;
}

//////////////////////////////////////////////////////////////////////////
//RZTypeConvert类
int RZTypeConvert::StrToInt(const std::string& _str, int _base)
{
	if (_str == "")
		Log::ERR("Pass a null string into this function.\n");
	if (_base <2 || _base > 16)
		Log::ERR("Unsupport convertion which base is larger than 16.\n");
	char* pEnd;
	return ::strtol(_str.c_str(), &pEnd, _base);
}

std::string RZTypeConvert::IntToString(int _i)
{
	char buf[16];
	sprintf(buf, "%d", _i);
	return std::string(buf);
}

//////////////////////////////////////////////////////////////////////////
//RZTime类
unsigned long RZTime::GetTimeStamp()
{
	//单位为毫秒
#ifdef WIN32
	return ::GetTickCount();
#elif
	struct timeval now;
	if (::gettimeofday(&now, NULL) == -1)
		Log::ERR("Platform SDK \'gettimeofday\' called failed.\n");
	return now.tv_sec*1000+now.tv_usec/1000;
#endif
}

unsigned long RZTime::GetWallClockTime()
{
	//单位为毫秒
#ifdef WIN32
	SYSTEMTIME sys;
	::GetLocalTime(&sys);
	return (sys.wHour*CHour2MilliSec+sys.wMinute*CMin2MilliSec+sys.wSecond*CSec2MilliSec+sys.wMilliseconds);
#elif

#endif
}

void RZTime::Sleep(unsigned long _ulDelay)
{
#ifdef WIN32
	::Sleep(_ulDelay);		//毫妙
#elif
	::usleep(1000*_ulDelay);
#endif
}

//////////////////////////////////////////////////////////////////////////
//RZBitmap类
RZBitmap::RZBitmap(unsigned long _nBits /* = CPoolSlots */)
:m_nBits(_nBits),
 m_nBytes((m_nBits+7)/8)
{
	m_pBitmap = (char*)malloc(m_nBytes);
	if (m_pBitmap == NULL)
		Log::ERR("malloc for pBitmap failed.\n");
	Clear();
}

RZBitmap::~RZBitmap()
{
	if (m_pBitmap != NULL)
	{
		free(m_pBitmap);
		m_pBitmap = NULL;
	}
}

int RZBitmap::CountOnByte(char _c) const
{
	int iCount = 0;
	while(_c)
	{
		_c &= (_c-1);		//去掉最低位的1
		iCount++;
	}
	return iCount;
}

unsigned long RZBitmap::BitCounts() const
{
	unsigned long nCount = 0;
	for (unsigned long i = 0; i < m_nBytes; i++)
		nCount += CountOnByte(m_pBitmap[i]);
	return nCount;
}

bool RZBitmap::GetBit(unsigned long _bit) const
{
	if (_bit >= m_nBits)
		Log::ERR("The bit want to get is Out of Range.\n");
	if ((m_pBitmap[_bit/8] & (1<<(_bit%8))) != 0)
		return true;
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////
//RZNetStrPool类
RZNetStrPool::RZNetStrPool(unsigned long _nSlots /* = CPoolSlots */)
	:m_nSlots(_nSlots)
{
	m_stBufPool.pBufferPool = (char*)malloc(m_nSlots*CPoolSlotSize);
	m_stBufPool.pSize = (unsigned long*)malloc(m_nSlots*sizeof(unsigned long));
	if (m_stBufPool.pBufferPool == NULL || m_stBufPool.pSize == NULL)
		Log::ERR("malloc for net string pool failed.\n");
	Clear();
}

RZNetStrPool::~RZNetStrPool()
{
	if (m_stBufPool.pBufferPool != NULL)
	{
		free(m_stBufPool.pBufferPool);
		m_stBufPool.pBufferPool = NULL;
	}
	if (m_stBufPool.pSize != NULL)
	{
		free(m_stBufPool.pSize);
		m_stBufPool.pSize = NULL;
	}
}

void RZNetStrPool::Insert(unsigned long _index, const char* _pSrc, unsigned long _ulSize)
{
	if (_index >= m_nSlots)
		Log::ERR("Index is Out of NetStrPool Range.\n");

	if ((m_stBufPool.pSize)[_index] == 0)
	{
		::memcpy(m_stBufPool.pBufferPool+_index*CPoolSlotSize, _pSrc, _ulSize);
		(m_stBufPool.pSize)[_index] = _ulSize;
		m_nItems++;
	}
}

//////////////////////////////////////////////////////////////////////////
//RZSemaphore类
RZSemaphore::RZSemaphore(long _init, long _ulMax)
{
#ifdef WIN32
	m_hSemaphore = ::CreateSemaphore(
		NULL,				//default security attributes
		_init,				//initial count
		_ulMax,			//maximum count
		NULL);			//unnamed semaphore
	if (m_hSemaphore == NULL)
		Log::ERR("PlatForm SDK \'CreateSemaphore\' called failed.\tError Code: %d.\n", ::GetLastError());
#endif
}

RZSemaphore::~RZSemaphore()
{
#ifdef WIN32
	if (m_hSemaphore != NULL)
		CloseHandle(m_hSemaphore);
#endif
}

void RZSemaphore::Wait(WAIT_MODE _wm /* = ENUM_SYN */, unsigned long _ulMilliSeconds /* = 0 */)
{
#ifdef WIN32
	DWORD dwWaitResult;
	if (_wm == ENUM_SYN)		//同步等待
		dwWaitResult = ::WaitForSingleObject(m_hSemaphore, INFINITE);
	else		//_wm == ENUM_ASYN
		dwWaitResult = ::WaitForSingleObject(m_hSemaphore, _ulMilliSeconds);
	if (dwWaitResult == WAIT_FAILED)
		Log::ERR("Platform SDK \'WaitForSingleObject\' called failed.\tError Code: %d.\n", GetLastError());
#endif
}

void RZSemaphore::Release()
{
#ifdef WIN32
	BOOL bRelease = ::ReleaseSemaphore(
										m_hSemaphore,		//handle to semaphore
										1,								//increase count by one
										NULL);					//not interested int previous count
	if (bRelease == false)
		Log::ERR("Platform SDK \'ReleaseSemaphore\' called failed.\tError Code: %d.\n", GetLastError());
#endif
}

//////////////////////////////////////////////////////////////////////////
//RZThread类
RZThread::RZThread()
	:m_ulThreadID(0)
{
}

RZThread::~RZThread()
{
#ifdef WIN32
	::CloseHandle(m_hThread);
#endif
}

#ifdef	WIN32
unsigned long RZThread::InitThreadProc(void* lpdwThreadParam)
{
	srand(static_cast<unsigned long>(time(NULL)));
	((RZThread*)lpdwThreadParam)->ThreadProc();
	//在线程退出之后可以做一些收尾工作
	return 0;
}
#endif

void RZThread::StartThread()
{
	//在开启线程前可以做一些初始化工作
#ifdef	WIN32
		m_hThread = CreateThread(NULL,			//Choose default security
			0,					//Default stack size
			(LPTHREAD_START_ROUTINE)&InitThreadProc,		//线程入口
			this,				//线程参数
			0,					//Immediately run the thread 
			&m_ulThreadID		//Thread ID
			);
		if (m_hThread == NULL)
			Log::ERR("Creating Thread Error. Error Code: %u\n", GetLastError());
#endif
}

void RZThread::WaitPeerThreadStop(const RZThread& rThread)
{
#ifdef WIN32
	::WaitForSingleObject(rThread.m_hThread, INFINITE);
#endif
}

//////////////////////////////////////////////////////////////////////////
//RZAgent类
RZAgent::RZAgent(RZNetConn* _pNetConn)
{
	if (_pNetConn == NULL)
		Log::ERR("Forbid set the point \'pNetConn\' NULL.\n");
	m_pNetConn = _pNetConn;
}

RZAgent::~RZAgent()
{
	if (m_pNetConn != NULL)
		delete m_pNetConn;
	m_pNetConn = NULL;
}

int RZAgent::RecvPeerData(WAIT_MODE _eWaitMode /* = ENUM_SYN */, long _milliSec /* = 0 */)
{
	int iCount = m_pNetConn->WaitForPeerResponse(_eWaitMode, _milliSec);
	if (iCount > 0)
	{
		NetString stNetStr = m_pNetConn->RecvDataFromPeer();
		ParsePacket(stNetStr.pBuffer, stNetStr.iSize);
	}
	return iCount;
}

void RZAgent::SetLocalPort(const RZNetPort& _port)
{
	//pNetConn must point to a concrete object, as RZNetConn is a pure virtual class
	if (m_pNetConn->GetPrtType() == ENUM_TCP)
		printf("[WARNING] Tcp port is automatic selected by system, this set operation will not be excuted.\n");
	else		//m_pNetConn->GetPrtType() == ENUM_UDP
	{
		RZUdpConn* pUdpConn = dynamic_cast<RZUdpConn*>(m_pNetConn);
		pUdpConn->InitLocalPort(_port);
	}
}

void RZAgent::ConnetToPeer(const RZNetIPAddr& _ip, const RZNetPort& _port)
{
	m_pNetConn->SetPeerIPAndPort(_ip, _port);
	m_pNetConn->BuildConnection();
}