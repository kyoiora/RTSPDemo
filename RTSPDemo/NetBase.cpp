#ifdef WIN32
#include <WS2tcpip.h>
#include <IPHlpApi.h>
#elif
#endif

#include "NetBase.h"
#include "MetaObject.h"

#pragma comment(lib, "iphlpapi.lib")

std::vector<unsigned long> RZNetPort::m_vPortList;

//////////////////////////////////////////////////////////////////////////
//RZNetPort类
RZNetPort::RZNetPort(NET_PRTTYPE _ePrtType)
	:m_port(0),
	 m_ePrtType(_ePrtType)
{
}

RZNetPort::RZNetPort(unsigned short _port, NET_PRTTYPE _ePrtType)
	:m_ePrtType(_ePrtType)
{
	//利用指定端口值调用这个构造函数时，禁止将ePrtType设置为ENUM_UNK
	SetPortValue(_port);
}

NET_PORTTYPE RZNetPort::GetPortType() const
{
	if (m_port >=0 && m_port <= 1023)			//知名端口范围为0~1023
		return ENUM_WELLKNOWN;
	else if (m_port >= 1024 && m_port <= 49151)		//注册端口范围为1024~49151
		return ENUM_REGISTER;
	else if (m_port >= 49152 && m_port <= 65535)	//动态端口范围为49152~65535
		return ENUM_DYNAMIC;
	else
		return ENUM_PORTTYPE_UNK;
}

bool RZNetPort::PortOccupied(unsigned long _port)
{
	if (_port > 65535)
		Log::ERR("Test port is not valid.\n");
	for (std::vector<unsigned long>::const_iterator iter = m_vPortList.begin(); 
			iter != m_vPortList.end(); iter++)
	{
		if (_port == *iter)		//当前端口和其中某个已使用端口相同，表明被占用
			return true;
	}
	return false;
}

std::vector<unsigned long> RZNetPort::GetLocalUsedPort(NET_PRTTYPE _pt)
{
	if (_pt == ENUM_TCP)
		return GetLocalUsedTcpPort();
	else if (_pt == ENUM_UDP)
		return GetLocalUsedUdpPort();
	else
		return std::vector<unsigned long>();
}

std::vector<unsigned long> RZNetPort::GetLocalUsedTcpPort()
{
	std::vector<unsigned long> vTcpPortList;

#ifdef WIN32
	ULONG dwRetVal, dwSize = sizeof(MIB_TCPTABLE);
	PMIB_TCPTABLE pTcpTable = 0;
	if ((pTcpTable = (PMIB_TCPTABLE)malloc(dwSize)) == 0)
		Log::ERR("malloc for \'PMIB_TCPTABLE\' failed.\n");

	//一般在首次调用GetTcpTable时都会返回错误码ERROR_INSUFFICIENT_BUFFER，它指示
	//缓冲区太小，无法装下足够多的TcpTable，因此首先调用该函数获取TcpTable的大小
	if (::GetTcpTable(pTcpTable, &dwSize, true) == ERROR_INSUFFICIENT_BUFFER)
	{
		free(pTcpTable);
		if ((pTcpTable = (PMIB_TCPTABLE)malloc(dwSize)) == 0)
			Log::ERR("malloc for \'PMIB_TCPTABLE\' failed.\n");
	}

	if ((dwRetVal = ::GetTcpTable(pTcpTable, &dwSize, true)) != NO_ERROR)
		Log::ERR("Platform SDK \'GetTcpTable\' failed.\tError Code: %u\n", dwRetVal);
	for (unsigned long i = 0; i < pTcpTable->dwNumEntries; i++)
		vTcpPortList.push_back(pTcpTable->table[i].dwLocalPort);
#endif
	return vTcpPortList;
}

std::vector<unsigned long> RZNetPort::GetLocalUsedUdpPort()
{
	std::vector<unsigned long> vUdpPortList;
#ifdef WIN32
	ULONG dwRetVal, dwSize = sizeof(MIB_UDPTABLE);
	PMIB_UDPTABLE pUdpTable = 0;
	if ((pUdpTable = (PMIB_UDPTABLE)malloc(dwSize)) == 0)
		Log::ERR("malloc for \'PMIB_UDPTABLE\' failed.\n");

	if (::GetUdpTable(pUdpTable, &dwSize, true) == ERROR_INSUFFICIENT_BUFFER)
	{
		free(pUdpTable);
		if ((pUdpTable = (PMIB_UDPTABLE)malloc(dwSize)) == 0)
			Log::ERR("malloc for \'PMIB_UDPTABLE\' failed.\n");
	}

	if ((dwRetVal = ::GetUdpTable(pUdpTable, &dwSize, true)) != NO_ERROR) 
		Log::ERR("Platform SDK \'GetUdpTable\' failed.\tError Code: %u\n", dwRetVal);
	for (unsigned long i = 0; i < pUdpTable->dwNumEntries; i++)
		vUdpPortList.push_back(pUdpTable->table[i].dwLocalPort);
#endif
	return vUdpPortList;
}

void RZNetPort::RandSelectValidPort(NET_PORTTYPE _pt)
{
	if (m_ePrtType == ENUM_PRT_UNK)
		Log::ERR("Unvalid Protocol Type. Please set protocol type first.\n");
	//只能在注册或者动态端口中进行随机选择
	if (_pt != ENUM_REGISTER && _pt != ENUM_DYNAMIC)
		Log::ERR("Please set a valid port type, then call this function.\n");

	int randNum;
	unsigned long rangeStart, rangeEnd;
	if (_pt == ENUM_REGISTER)		//使用注册端口
	{
		rangeStart = CRPRangeStart;
		rangeEnd = CRPRangeEnd;
	} 
	else //使用动态端口
	{
		rangeStart = CDPRangeStart;
		rangeEnd = CDPRangeEnd;
	}

	UpdateSystemOccupiedPort();			//更新当前系统占用端口列表
	do
	{
		randNum = rand()%(rangeEnd-rangeStart+1)+rangeStart;
		if (randNum%2 == 1)		//端口为奇数（RTP协议约定使用偶数端口）
			randNum++;
	} while(PortOccupied(randNum));
	m_port = randNum;
}

void RZNetPort::SetPortValue(unsigned short _port)
{
	if (m_ePrtType == ENUM_PRT_UNK)
		Log::ERR("Unvalid Protocol Type. Please set protocol type first.\n");

	UpdateSystemOccupiedPort();			//更新系统占用端口
	if (PortOccupied(_port))			//端口被占用
		m_port = 0;
	else
		m_port = _port;
}

//////////////////////////////////////////////////////////////////////////
//RZNetIPAddr类
RZNetIPAddr::RZNetIPAddr()
	:m_uIPAddr(0),
	 m_strIPAddr("")
{
}

RZNetIPAddr::RZNetIPAddr(const std::string& _str)
{
	if (!IPAddrLegal(_str))		//IP地址格式非法
		Log::ERR("Input an unvalid IP Address.\n");
	InitIPAddr(_str);
}

bool RZNetIPAddr::IPAddrLegal(const std::string& _str) const
{
	//将点分十进制IP地址按照"."进行分割，将所有的原子字符串存入vector容器
	std::vector<std::string> vAtomList = RZStream::StreamSplit(_str, ".");
	for (std::vector<std::string>::const_iterator iter = vAtomList.begin(); 
			iter != vAtomList.end(); iter++)
	{
		int iSubField = RZTypeConvert::StrToInt(*iter, 10);		//以十进制形式转换
		if (iSubField < 0 || iSubField > 255)		//每个子域在0~255之间
			return false;
	}
	return true;
}

void RZNetIPAddr::SetIPAddr(const std::string& _str)
{
	if (!IPAddrLegal(_str))		//IP地址格式非法
		Log::ERR("Input an unvalid IP Address.\n");
	InitIPAddr(_str);
}

//////////////////////////////////////////////////////////////////////////
//RZReqLine类
RZReqLine::RZReqLine(APP_PRTTYPE _eAppPt)
	:m_strMethod(""),
	m_strUri("")
{
	switch (_eAppPt)
	{
	case ENUM_RTSP:	m_strVersion = "RTSP/1.0";break;
		//...
	default:
		Log::ERR("Unknown Application Protocol Type.\n");
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
//RZExtraHdr类
RZExtraHdr::RZExtraHdr()
	:m_strHdrName(""),
	 m_strHdrData("")
{
}

//////////////////////////////////////////////////////////////////////////
//RZReqPacket类
RZReqPacket::RZReqPacket(APP_PRTTYPE _eAppPt)
	:m_rLine(_eAppPt),
	 m_vExtHdrList()
{
}

std::string RZReqPacket::GetRequestString() const
{
	//一个请求串的格式为：
	 //<request line>\r\n
	 //<extra header>\r\n
	 //more extra header
	 //\r\n
	 //
	if (m_rLine.GetReqLine() == "")
		Log::ERR("Request Line can not be null.\n");
	std::string str = m_rLine.GetReqLine()+"\r\n";
	for (std::vector<RZExtraHdr>::const_iterator iter = m_vExtHdrList.begin();
			iter != m_vExtHdrList.end(); iter++)
	{
		if (iter->GetExtraHdr() == "")
			continue;		//压入空的附加请求头时，不在str中加入额外的\r\n
		str += iter->GetExtraHdr()+"\r\n";
	}
	str += "\r\n";		//串结束符标志
	return str;
}