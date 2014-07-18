#ifndef	CONNECTION_H_
#define	CONNECTION_H_

#ifdef WIN32
#include <WinSock2.h>
#endif

#include <string>
#include "NetBase.h"

const unsigned long CRecvBufSize = 4096;

struct _NetString
{
	char* pBuffer;
	int iSize;

	_NetString(char* _pBuffer)
		:pBuffer(_pBuffer),
		 iSize(0)
	{
	}
};
typedef struct _NetString		NetString;

typedef enum
{
	ENUM_SYN = 0,		//同步模式
	ENUM_ASYN				//异步模式
} WAIT_MODE;

//////////////////////////////////////////////////////////////////////////
class RZNetConn 
{
public:
	RZNetConn();
	virtual ~RZNetConn();

	virtual void BuildConnection() = 0;
	virtual void SendDataToPeer(const char*, int) = 0;
	virtual NetString RecvDataFromPeer() = 0;

public:
	int GetSysRecvBufSize() const;
	void SetSysRecvBufSize(int);
	void SetLocalBufSize(unsigned long);

	inline RZNetIPAddr GetPeerIP() const;
	inline RZNetPort GetPeerPort() const;
	inline NET_PRTTYPE GetPrtType() const;
	inline void SetPeerIPAndPort(const RZNetIPAddr&, const RZNetPort&);

	//////////////////////////////////////////////////////////////////////////
	inline void ClearReadSet();
	inline void AppSockInReadSet();
	int WaitForPeerResponse(const WAIT_MODE&, long _milliSec = 0) const;

private:
	void InitNetConn();

#ifdef	WIN32
	void InitWinSockDLL();
#endif

private:
	fd_set m_readSet;
	int m_maxFd;		//读集合的最大文件描述符

protected:
	void CreateSocket();
	inline bool SocketValid() const;

protected:
	int m_iSockFile;		//iSockFile = -1 无效；否则有效
	RZNetIPAddr	m_nIPAddr;		//n means net
	RZNetPort			m_nPort;

	char* m_rBuffer;			//r means receive
	unsigned long m_uBufSize;
};

inline void RZNetConn::ClearReadSet()
{
	FD_ZERO(&m_readSet);
	m_maxFd = 0;
}

inline void RZNetConn::AppSockInReadSet()
{
	if (!SocketValid())
		Log::ERR("Socket is not valid, please create socket first.\n");

	FD_SET(m_iSockFile, &m_readSet);
	m_maxFd = max(m_maxFd, static_cast<int>(m_iSockFile));
}

inline bool RZNetConn::SocketValid() const
{
	if (m_iSockFile != -1)		//不等于-1，有效
		return true;
	else
		return false;
}

inline RZNetIPAddr RZNetConn::GetPeerIP() const
{
	if (!m_nIPAddr.IPAddrValid())
		Log::ERR("IP Address is not valid, Please set IP Address first.\n");
	return m_nIPAddr;
}

inline RZNetPort RZNetConn::GetPeerPort() const
{
	if (!m_nPort.PortValid())
		Log::ERR("Port is not valid, Please set port first.\n");
	return m_nPort;
}

inline NET_PRTTYPE RZNetConn::GetPrtType() const
{
	if (m_nPort.GetPrtType() == ENUM_PRT_UNK)
		Log::ERR("Unknown transport protocol type, get protocol type failed.\n");
	return m_nPort.GetPrtType();
}

inline void RZNetConn::SetPeerIPAndPort(const RZNetIPAddr& _ip, const RZNetPort& _port)
{
	if (!_ip.IPAddrValid() || !_port.PortValid())
		Log::ERR("IP Address or Port is not valid, Please set a valid value.\n");
	m_nIPAddr = _ip;
	m_nPort = _port;
}

//////////////////////////////////////////////////////////////////////////
class RZTcpConn : public RZNetConn 
{
public:
	RZTcpConn();
	~RZTcpConn() {}

public:
	void BuildConnection();
	void SendDataToPeer(const char*, int);
	NetString RecvDataFromPeer();
};

//////////////////////////////////////////////////////////////////////////
class RZUdpConn : public RZNetConn 
{
public:
	RZUdpConn();
	~RZUdpConn() {}

public:
	inline void InitLocalPort(const RZNetPort&);
	inline RZNetPort GetLocalPort() const;			//获取本地接收数据的端口

	void BuildConnection();
	void SendDataToPeer(const char*, int);
	NetString RecvDataFromPeer();

private:
	void BindLocalPort(const RZNetPort&);			//设置本地接收数据的端口

private:
	struct sockaddr_in m_stPeerAddr;
	RZNetPort m_uLocalPort;

	struct sockaddr_in m_lPeerAddr;
	int m_iSize;
};

inline void RZUdpConn::InitLocalPort(const RZNetPort& _port)
{
	if (_port.GetPrtType() != ENUM_UDP)
		Log::ERR("Only UDP protocol need to initialize local port, initialize failed.\n");

	BindLocalPort(_port);
	AppSockInReadSet();		//绑定时创建套接字
}

inline RZNetPort RZUdpConn::GetLocalPort() const
{
	return m_uLocalPort;
}
#endif		//CONNECTION_H_