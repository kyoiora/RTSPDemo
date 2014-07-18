#include <stdlib.h>
#include "NetConn.h"
#include "Logger.h"

#ifdef	WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

//////////////////////////////////////////////////////////////////////////
//RZNetConn类
RZNetConn::RZNetConn()
	:m_iSockFile(-1), 
	 m_nIPAddr(),
	 m_nPort(ENUM_PRT_UNK),
	 m_uBufSize(CRecvBufSize)
{
	InitNetConn();
}

RZNetConn::~RZNetConn()
{
#ifdef WIN32
	if (m_iSockFile != -1)		//套接字有效
		::closesocket(m_iSockFile);		//关闭套接字
	if (WSACleanup() == SOCKET_ERROR)
		Log::ERR("clean up resource used by socket file failed.\n");
#endif
	free(m_rBuffer);		//释放缓冲区所占资源
}

void RZNetConn::InitNetConn()
{
	if ((m_rBuffer = (char*)malloc(m_uBufSize)) == 0)		//申请缓冲区
		Log::ERR("malloc for net connection failed.\n");
	::memset(m_rBuffer, 0, m_uBufSize);
	ClearReadSet();		//清空读集合

#ifdef WIN32
	InitWinSockDLL();
#endif
}

void RZNetConn::SetLocalBufSize(unsigned long _uBufSize)
{
	void* v = realloc(m_rBuffer, _uBufSize);
	if (v == 0)
		Log::ERR("realloc for rBuffer failed.\n");
	m_rBuffer = (char*)v;
	m_uBufSize = _uBufSize;
}

int RZNetConn::GetSysRecvBufSize() const
{
	if (!SocketValid())
		Log::ERR("Please create a socket file first.\n");

#ifdef WIN32
	int optVal, optLen = sizeof(int);
	if (::getsockopt(m_iSockFile, SOL_SOCKET, SO_RCVBUF, 
								(char*)&optVal, &optLen) == SOCKET_ERROR)
	{
		Log::ERR("Platform SDK \'getsockopt\' called failed.\tError Code: %d\n", 
							WSAGetLastError());
	}
	return optVal;
#endif
}

void RZNetConn::SetSysRecvBufSize(int _iBufSize)
{
	if (!SocketValid())
		Log::ERR("Please create a socket file first.\n");

#ifdef WIN32
	if (::setsockopt(m_iSockFile, SOL_SOCKET, SO_RCVBUF, 
								(const char*)&_iBufSize, sizeof(int)) == SOCKET_ERROR)
	{
		Log::ERR("Platform SDK \'setsockopt\' called failed.\tError Code: %d\n", 
							WSAGetLastError());
	}
#endif
}

int RZNetConn::WaitForPeerResponse(const WAIT_MODE& _wm, long _milliSec /* = 0 */) const
{
	if (_wm != ENUM_SYN && _wm != ENUM_ASYN)
		Log::ERR("Select incorrect wait mode.\n");

	fd_set readySet = m_readSet;
	struct timeval tv, *ptv;
	int iRet;
	if (_wm == ENUM_SYN)		//同步模式
		ptv = NULL;
	else		//异步模式
	{
		tv.tv_sec = 0;
		tv.tv_usec = _milliSec*1000;		//_milliSec表示毫秒，而tv_usec字段表示微妙
		ptv = &tv;
	}
#ifdef	WIN32
	if ((iRet = ::select(m_maxFd+1, &readySet, NULL, NULL, ptv)) == SOCKET_ERROR)
	{
		Log::ERR("Platform SDK \'Select\' called failed.\tError Code: %d", WSAGetLastError());
	}
#endif
	return iRet;
}

#ifdef WIN32
void RZNetConn::InitWinSockDLL()
{
	WSADATA wsa;
	::memset(&wsa, 0, sizeof(WSADATA));
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		Log::ERR("Initialize socket file failed.\n");
}
#endif

void RZNetConn::CreateSocket()
{
	if (m_nPort.GetPrtType() != ENUM_TCP && m_nPort.GetPrtType() != ENUM_UDP)
		Log::ERR("Please set transport protocol first.\n");

#ifdef WIN32
	int iType, iProtocol, iOptVal = 1;
	if (m_nPort.GetPrtType() == ENUM_TCP)			//TCP
	{
		iType = SOCK_STREAM;
		iProtocol = IPPROTO_TCP;
	} 
	else			//UDP
	{
		iType = SOCK_DGRAM;
		iProtocol = IPPROTO_UDP;
	}
	if ((m_iSockFile = ::socket(AF_INET,iType, iProtocol)) == INVALID_SOCKET)
		Log::ERR("Platform SDK \'socket\' called failed.\tError Code: %d\n", WSAGetLastError());

	if (::setsockopt(m_iSockFile, SOL_SOCKET, SO_REUSEADDR, (const char*)&iOptVal, sizeof(int)) < 0)
		Log::ERR("Platform SDK \'setsockopt\' called failed.\tError Code: %d\n", WSAGetLastError());
#endif
}

//////////////////////////////////////////////////////////////////////////
//RZTcpConn类
RZTcpConn::RZTcpConn()
	:RZNetConn()
{
	m_nPort.SetPrtType(ENUM_TCP);
}

void RZTcpConn::BuildConnection()
{
	if (!m_nIPAddr.IPAddrValid() || !m_nPort.PortValid())		//只要两者之一无效，那么无法建立连接
		Log::ERR("IP Address or Port is not valid, can not build connection.\n");

	CreateSocket();		//创建套接字
	AppSockInReadSet();
#ifdef WIN32
	//填充套接字地址结构
	struct sockaddr_in stPeerAddr;
	::memset(&stPeerAddr, 0, sizeof(stPeerAddr));
	stPeerAddr.sin_family = AF_INET;
	stPeerAddr.sin_addr.S_un.S_addr = m_nIPAddr.GetULIPAddr();
	stPeerAddr.sin_port = ::htons(m_nPort.GetPortValue());

	if (::connect(m_iSockFile, (sockaddr*)&stPeerAddr, sizeof(stPeerAddr)) ==SOCKET_ERROR) 
		Log::ERR("Platform SDK \'connect\' called failed.\tError Code: %d\n", WSAGetLastError());
#endif
}

void RZTcpConn::SendDataToPeer(const char* _buf, int _size)
{
	if (!SocketValid())
		Log::ERR("Can not send data, Please BuildConnection first.\n");

#ifdef WIN32
	if (::send(m_iSockFile, _buf, _size, 0) == SOCKET_ERROR)
		Log::ERR("Platform SDK \'send\' called failed.\tErrorCode: %d\n", WSAGetLastError());
#endif
}

NetString RZTcpConn::RecvDataFromPeer()
{
	if (!SocketValid())
		Log::ERR("Can not receive data, Please BuildConnection first.\n");
	
	NetString stNetStr(m_rBuffer);
	::memset(m_rBuffer, 0, m_uBufSize);
#ifdef WIN32
	if ((stNetStr.iSize = ::recv(m_iSockFile, m_rBuffer, m_uBufSize, 0)) == SOCKET_ERROR)
		Log::ERR("Platform SDK \'recv\' called failed.\tError Code: %d\n", WSAGetLastError());
#endif

	return stNetStr;
}

//////////////////////////////////////////////////////////////////////////
//RZUdpConn类
RZUdpConn::RZUdpConn()
	:RZNetConn(), 
	 m_uLocalPort(ENUM_UDP),
	 m_iSize(sizeof(sockaddr_in))
{
	m_nPort.SetPrtType(ENUM_UDP);
	::memset(&m_lPeerAddr, 0, sizeof(sockaddr_in));
}

void RZUdpConn::BindLocalPort(const RZNetPort& _port)
{
	if (!_port.PortValid())
		Log::ERR("Port is not valid, set local listen port failed.\n");
	m_uLocalPort = _port;

	CreateSocket();		//创建套接字
#ifdef WIN32
	struct sockaddr_in localAddr;
	::memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.S_un.S_addr = ::htonl(INADDR_ANY);		//接收任何地址发来的数据包
	localAddr.sin_port = ::htons(m_uLocalPort.GetPortValue());
	//简单绑定就可以了，UDP是“无连接”协议
	if (::bind(m_iSockFile, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR)
		Log::ERR("Platform SDK \'bind\' called failed.\tError Code: %d\n", ::WSAGetLastError());
#endif
}

void RZUdpConn::BuildConnection()
{
	if (!m_uLocalPort.PortValid())
		Log::ERR("Local listen port has not binded, please call \'InitLocalPort\' first.\n");
	if (!m_nIPAddr.IPAddrValid() || !m_nPort.PortValid())		//只要两者之一无效，那么无法建立连接
		Log::ERR("IP Address or Port is not valid, can not build connection.\n");

#ifdef WIN32
	::memset(&m_stPeerAddr, 0, sizeof(m_stPeerAddr));
	m_stPeerAddr.sin_family = AF_INET;
	m_stPeerAddr.sin_addr.S_un.S_addr = m_nIPAddr.GetULIPAddr();
	m_stPeerAddr.sin_port = ::htons(m_nPort.GetPortValue());
#endif
}

void RZUdpConn::SendDataToPeer(const char* _buf, int _size)
{
	if (!SocketValid())
		Log::ERR("Can not send data, Please BuildConnection first.\n");

#ifdef WIN32
	if (::sendto(m_iSockFile, _buf, _size, 0, (sockaddr*)&m_stPeerAddr, sizeof(sockaddr)) == SOCKET_ERROR)
		Log::ERR("Platform SDK \'sendto\' called failed.\tError Code: %d\n", WSAGetLastError());
#endif
}

NetString RZUdpConn::RecvDataFromPeer()
{
	if (!SocketValid())
		Log::ERR("Can not receive data, Please BuildConnection first.\n");

	NetString stNetStr(m_rBuffer);
	::memset(m_rBuffer, 0, m_uBufSize);
#ifdef WIN32
	stNetStr.iSize = ::recvfrom(m_iSockFile, m_rBuffer, m_uBufSize, 0,
													(sockaddr*)&m_lPeerAddr, &m_iSize);
	if (stNetStr.iSize == SOCKET_ERROR)
		Log::ERR("Platform SDK \'recvfrom\' called failed.\tError Code: %d\n", WSAGetLastError());
#endif

	return stNetStr;
}