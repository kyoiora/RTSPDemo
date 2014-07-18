#ifndef	NETBASE_H_
#define	NETBASE_H_

#ifdef WIN32
#include <WinSock2.h>
#endif

#include <string>
#include <vector>
#include "Logger.h"

const unsigned long CRPRangeStart = 2048;			//用于随机选择的端口范围
const unsigned long CRPRangeEnd = 48512;
const unsigned long CDPRangeStart = 49256;
const unsigned long CDPRangeEnd = 65256;

typedef enum 
{
	ENUM_TCP = 0,
	ENUM_UDP, 
	ENUM_PRT_UNK		//未知类型
} NET_PRTTYPE;

typedef enum 
{
	ENUM_WELLKNOWN = 0,		//知名端口，从0~1023，提供给一些知名服务使用
	ENUM_REGISTER,					//注册端口，从1024~49151，提供给用户进程或应用程序使用
	ENUM_DYNAMIC,				//动态端口，从49152~65535，不固定分配给某种服务，动态分配
	ENUM_PORTTYPE_UNK
} NET_PORTTYPE;

typedef enum 
{
	ENUM_RTSP = 0, 
	//...
} APP_PRTTYPE;

class RZNetPort 
{
public:
	RZNetPort(NET_PRTTYPE);		//强制以指定协议方式初始化
	RZNetPort(unsigned short, NET_PRTTYPE);
	~RZNetPort() {}

public:
	inline bool PortValid() const;
	inline NET_PRTTYPE GetPrtType() const;				//获取协议类型
	inline void SetPrtType(NET_PRTTYPE);				//设置协议类型
	inline unsigned short GetPortValue() const;		//取得端口的值
	void SetPortValue(unsigned short);						//设置端口的值
	void RandSelectValidPort(NET_PORTTYPE);

private:
	bool PortOccupied(unsigned long);
	NET_PORTTYPE GetPortType() const;

	//////////////////////////////////////////////////////////////////////////
	inline void UpdateSystemOccupiedPort();
	std::vector<unsigned long> GetLocalUsedPort(NET_PRTTYPE);
	std::vector<unsigned long> GetLocalUsedTcpPort();
	std::vector<unsigned long> GetLocalUsedUdpPort();

private:
	unsigned short	m_port;
	NET_PRTTYPE	m_ePrtType;		//端口对应的协议类型

	//增加持久化对象，保存系统当前占用端口
	static std::vector<unsigned long> m_vPortList;
};

inline bool RZNetPort::PortValid() const
{
	if (m_port == 0)		//端口为0表示不可用
		return false;
	return true;
}

inline NET_PRTTYPE RZNetPort::GetPrtType() const
{
	return m_ePrtType;
}

inline void RZNetPort::SetPrtType(NET_PRTTYPE _ePrtType)
{
	if (_ePrtType == ENUM_PRT_UNK)
		Log::ERR("Unvalid Protocol Type. Please set protocol type first.\n");
	m_ePrtType = _ePrtType;
}

inline unsigned short RZNetPort::GetPortValue() const
{
	if (m_port == 0)
		Log::ERR("Current port is not valid, Please set a valid port first.\n");
	return m_port;
}

inline void RZNetPort::UpdateSystemOccupiedPort()
{
	m_vPortList = GetLocalUsedPort(m_ePrtType);
}

//////////////////////////////////////////////////////////////////////////
class RZNetIPAddr 
{
public:
	RZNetIPAddr();
	RZNetIPAddr(const std::string&);
	~RZNetIPAddr() {}

public:
	//取得IP地址的点分十进制串形式及无符号整形形式
	inline unsigned long GetULIPAddr() const;
	inline std::string GetSTRIPAddr() const;

	void SetIPAddr(const std::string&);
	inline bool IPAddrValid() const;

private:
	inline void InitIPAddr(const std::string&);
	bool IPAddrLegal(const std::string&) const;

private:
	unsigned long	m_uIPAddr;
	std::string			m_strIPAddr;
};

inline void RZNetIPAddr::InitIPAddr(const std::string& _str)
{
	m_strIPAddr = _str;
#ifdef WIN32
	m_uIPAddr = ::inet_addr(_str.c_str());
#endif
}

inline bool RZNetIPAddr::IPAddrValid() const
{
	if (m_uIPAddr != 0)
		return true;
	else
		return false;
}

inline unsigned long RZNetIPAddr::GetULIPAddr() const
{
	if (m_uIPAddr == 0)
		Log::ERR("Please set a valid IP Address first.\n");
	return m_uIPAddr;
}

inline std::string RZNetIPAddr::GetSTRIPAddr() const
{
	if (m_uIPAddr == 0)
		Log::ERR("Please set a valid IP Address first.\n");
	return m_strIPAddr;
}

//////////////////////////////////////////////////////////////////////////
class RZReqLine
{
public:
	RZReqLine(APP_PRTTYPE);
	~RZReqLine() {}

public:
	inline void SetReqLine(const std::string&, const std::string&);
	inline std::string GetReqLine() const;

private:
	//一个请求行的格式为<method> <uri> <version>
	std::string m_strMethod;
	std::string m_strUri;
	std::string m_strVersion;
};

inline void RZReqLine::SetReqLine(const std::string& _strMethod, const std::string& _strUri)
{
	m_strMethod = _strMethod;
	m_strUri = _strUri;
}

inline std::string RZReqLine::GetReqLine() const
{
	return m_strMethod+" "+m_strUri+" "+m_strVersion;
}

//////////////////////////////////////////////////////////////////////////
class RZExtraHdr 
{
public:
	RZExtraHdr();
	~RZExtraHdr() {}

public:
	inline void SetExtraHdr(const std::string&, const std::string&);
	inline std::string GetExtraHdr() const;

private:
	//附加报头的格式为<header name>: <header data>
	std::string m_strHdrName;
	std::string m_strHdrData;
};

inline void RZExtraHdr::SetExtraHdr(const std::string& _strHdrName, const std::string& _strHdrData)
{
	m_strHdrName = _strHdrName;
	m_strHdrData = _strHdrData;
}

inline std::string RZExtraHdr::GetExtraHdr() const
{
	return m_strHdrName+": "+m_strHdrData;
}

//////////////////////////////////////////////////////////////////////////
class RZReqPacket 
{
public:
	RZReqPacket(APP_PRTTYPE);
	~RZReqPacket() {}

public:
	inline void FillPacket(const RZReqLine&, const std::vector<RZExtraHdr>&);
	std::string GetRequestString() const;

private:
	//一个请求包含一个请求行和多个请求报头
	RZReqLine m_rLine;
	std::vector<RZExtraHdr> m_vExtHdrList;
};

inline void RZReqPacket::FillPacket(const RZReqLine& _rLine, const std::vector<RZExtraHdr>& _vHdrList)
{
	m_rLine = _rLine;
	m_vExtHdrList = _vHdrList;
}
#endif		//NETBASE_H_