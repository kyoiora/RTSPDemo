#include "RTSPAgent.h"

//v：①RTSPClient/0.0.1 (HP Streaming Media v2014.05.24)
const std::string CRTSPClientAgent = "RTSPClient/0.1.1 (HP Streaming Media v2014.07.01)";
const std::string CRTSPDesContType = "application/sdp";
const std::string CRTSPVersion = "RTSP/1.0";
//transport-spec = "transport-protocol/profile[/lower-transport]"
//transport-protocol = RTP,	profile = AVP,	lower-transport = TCP | UDP
//For RTP/AVP, the default lower-transport is UDP
const std::string CRTSPTransSpec = "RTP/AVP";


RZRTSPAgent::RZRTSPAgent()
:RZAgent(new RZTcpConn),
 m_strReqFile(),
 m_stResPacket(),
 m_stHdrCache(),
 m_sdpPacket(),
 m_pRTPAgent(NULL)
{
}

void RZRTSPAgent::ConnectToServer(const std::string& _ip)
{
	RZNetIPAddr	nIPAddr(_ip);
	RZNetPort			nPort(CRTSPDefaultPort, ENUM_TCP);
	ConnetToPeer(nIPAddr, nPort);
}

void RZRTSPAgent::StartTask(RTSP_TASK _eTask)
{
	m_eTask = _eTask;
	m_stHdrCache.iCseq = 0;
	m_stHdrCache.strSession = "";
	StartThread();
}

void RZRTSPAgent::ThreadProc()
{
	switch(m_eTask) 
	{
	case ENUM_ACCEPTAS:			OnDispatchAcceptAVStream();		break;
	case ENUM_ACCEPTVS:			OnDispatchAcceptAVStream();		break;
	case ENUM_ACCEPTAVS:		OnDispatchAcceptAVStream();		break;
		// ...
	}
}

std::string RZRTSPAgent::GetMethodStr(RTSP_METHOD _eMethod) const
{
	std::string strMethod;
	switch(_eMethod)
	{
	case ENUM_DESCRIBE:					strMethod = "DESCRIBE";						break;
	case ENUM_ANNOUNCE:				strMethod = "ANNOUNCE";					break;
	case ENUM_GET_PARAMETER:		strMethod = "GET_PARAMETER";		break;
	case ENUM_OPTIONS:					strMethod = "OPTIONS";						break;
	case ENUM_PAUSE:							strMethod = "PAUSE";							break;
	case ENUM_PLAY:							strMethod = "PLAY";								break;
	case ENUM_RECORD:						strMethod = "RECORD";						break;
	case ENUM_REDIRECT:					strMethod = "REDIRECT";						break;
	case ENUM_SETUP:							strMethod = "SETUP";							break;
	case ENUM_SET_PARAMETER:		strMethod = "SET_PARAMETER";			break;
	case ENUM_TEARDOWN:				strMethod = "TEARDOWN";					break;
	}
	return strMethod;
}

RZReqLine RZRTSPAgent::GetReqLineByMethod(RTSP_METHOD _eMethod, int _iSETUP)
{
	RZReqLine rLine(ENUM_RTSP);
	std::string strMethod = GetMethodStr(_eMethod);
	std::string strUri = "rtsp://"+m_pNetConn->GetPeerIP().GetSTRIPAddr()+":"+ \
		RZTypeConvert::IntToString(m_pNetConn->GetPeerPort().GetPortValue())+"/"+m_strReqFile;

	if (_eMethod == ENUM_SETUP)
	{
		std::string dataBlockID = "";
		switch(m_eTask) 
		{
		case ENUM_ACCEPTAS: dataBlockID = m_sdpPacket.GetAudioID(); break;
		case ENUM_ACCEPTVS: dataBlockID = m_sdpPacket.GetVideoID(); break;
		case ENUM_ACCEPTAVS:
			{
				if (_iSETUP == 0)
					dataBlockID = m_sdpPacket.GetAudioID();
				else
					dataBlockID = m_sdpPacket.GetVideoID();
				break;
			}
		}
		strUri += "/"+dataBlockID;
	}
	rLine.SetReqLine(strMethod, strUri);
	return rLine;
}

std::vector<RZExtraHdr> RZRTSPAgent::GetGeneralExtraHdr() const
{
	std::vector<RZExtraHdr> vExtraHdrList;
	RZExtraHdr extHdr;
	extHdr.SetExtraHdr("CSeq", RZTypeConvert::IntToString(m_stHdrCache.iCseq));
	vExtraHdrList.push_back(extHdr);			//①添加CSeq报头
	extHdr.SetExtraHdr("User-Agent", CRTSPClientAgent);
	vExtraHdrList.push_back(extHdr);			//②添加User-Agent报头
	if (m_stHdrCache.strSession != "")		//判断Session是否有效
	{
		extHdr.SetExtraHdr("Session", m_stHdrCache.strSession);
		vExtraHdrList.push_back(extHdr);			//添加Session报头
	}
	return vExtraHdrList;
}

void RZRTSPAgent::GetDESCRIBEExtraHdr(std::vector<RZExtraHdr>& _vExtraHdrList) const
{
	RZExtraHdr extHdr;
	switch (m_eTask) 
	{
	case ENUM_ACCEPTAS:
	case ENUM_ACCEPTVS:
	case ENUM_ACCEPTAVS:
		{
		extHdr.SetExtraHdr("Accept", CRTSPDesContType);
		_vExtraHdrList.push_back(extHdr);			//添加Accept报头
		break;
		}
	}
}

std::string RZRTSPAgent::GetTransHeader(int _iSETUP) const
{
	std::string strTrans = CRTSPTransSpec+";"+"unicast;";

	//在Transport报头中还需要设置client_port参数的值，这个值代表RTP/RTCP的本地接收端口
	unsigned short port = m_pRTPAgent[_iSETUP].GetLocalPort().GetPortValue();
	strTrans += "client_port="+RZTypeConvert::IntToString(port)+"-"+RZTypeConvert::IntToString(port+1);
	return strTrans;
}

void RZRTSPAgent::GetSETUPExtraHdr(std::vector<RZExtraHdr>& _vExtraHdrList, int _iSETUP) const
{
	//①_iSETUP = 0：为音频流建立SETUP的附加报头；②_iSETUP = 1：为视频流建立附加报头
	RZExtraHdr extHdr;
	switch (m_eTask) 
	{
	case ENUM_ACCEPTAS:
	case ENUM_ACCEPTVS:
	case ENUM_ACCEPTAVS:
		{
			extHdr.SetExtraHdr("Transport", GetTransHeader(_iSETUP));
			_vExtraHdrList.push_back(extHdr);			//添加Transport报头
		}
		break;
	}
}

void RZRTSPAgent::GetPLAYExtraHdr(std::vector<RZExtraHdr>& _vExtraHdrList) const
{
	RZExtraHdr extHdr;
	switch (m_eTask)
	{
	case ENUM_ACCEPTAS:
	case ENUM_ACCEPTVS:
	case ENUM_ACCEPTAVS:
		{
			//"npt=0.000-"表示从头开始接受媒体文件的数据流
			extHdr.SetExtraHdr("Range", "npt=0.000-");
			_vExtraHdrList.push_back(extHdr);				//添加Range报头
		}
		break;
	}
}

std::vector<RTSP_METHOD> RZRTSPAgent::GetTaskMethodList() const
{
	
	std::vector<RTSP_METHOD> vTskMethodList;
	vTskMethodList.push_back(ENUM_OPTIONS);
	vTskMethodList.push_back(ENUM_DESCRIBE);
	vTskMethodList.push_back(ENUM_SETUP);

	switch (m_eTask)
	{
	case ENUM_ACCEPTAS:
	case ENUM_ACCEPTVS:
		break;
	case ENUM_ACCEPTAVS:
		vTskMethodList.push_back(ENUM_SETUP);
		break;
	}

	vTskMethodList.push_back(ENUM_PLAY);
	vTskMethodList.push_back(ENUM_TEARDOWN);
	return vTskMethodList;
}

void RZRTSPAgent::SendRequest(RTSP_METHOD _eMethod, int _iSETUP)
{
	m_stHdrCache.iCseq++;

	RZReqPacket rPacket(ENUM_RTSP);
	RZReqLine rLine = GetReqLineByMethod(_eMethod, _iSETUP);
	std::vector<RZExtraHdr> vExtraHdrList = GetGeneralExtraHdr();
	switch (_eMethod)
	{
	case ENUM_DESCRIBE:			GetDESCRIBEExtraHdr(vExtraHdrList);					break;
	case ENUM_SETUP:					GetSETUPExtraHdr(vExtraHdrList, _iSETUP);		break;
	case ENUM_PLAY:					GetPLAYExtraHdr(vExtraHdrList);							break;
	default:		break;
	}
	rPacket.FillPacket(rLine, vExtraHdrList);
	std::string str = rPacket.GetRequestString();
	m_pNetConn->SendDataToPeer(str.c_str(), str.length());
}

void RZRTSPAgent::ParsePacket(const char* _str, unsigned long _ulSize)
{
	std::string str(_str, _ulSize);
	std::vector<std::string> vTextLine;
	std::vector<std::string> vTextLineList = RZStream::StreamSplit(str, "\r\n");		//以"\r\n分隔每个文本行"

	vTextLine = RZStream::StreamSplit(vTextLineList[0], " ");		//响应行的每个域以空格分隔
	if (vTextLine[0] != CRTSPVersion)
		Log::ERR("RTSP Version is not correct, but this case doesn't happen usually.\n");
	m_stResPacket.strStatusCode = vTextLine[1];		//取状态码

	std::vector<std::string>::const_iterator iter = vTextLineList.begin()+1;
	for (; iter != vTextLineList.end(); iter++)
	{
		vTextLine = RZStream::StreamSplit(*iter, ": ");		//每个header的域和值以": "分隔
		if (vTextLine.size() != 2) break;		//容器中的元素不是2，说明报头全部解析完毕

		//通用报头
		if			(vTextLine[0] == "CSeq")								m_stResPacket.stGrlHdr.ghCseq =								vTextLine[1];
		else if	(vTextLine[0] == "Cache-Control")				m_stResPacket.stGrlHdr.ghCacheControl =				vTextLine[1];
		else if	(vTextLine[0] == "Connection")					m_stResPacket.stGrlHdr.ghConnection =					vTextLine[1];
		else if	(vTextLine[0] == "Date")								m_stResPacket.stGrlHdr.ghDate =								vTextLine[1];
		else if	(vTextLine[0] == "Via")									m_stResPacket.stGrlHdr.ghVia =									vTextLine[1];
		else if	(vTextLine[0] == "Scale")								m_stResPacket.stGrlHdr.ghScale =								vTextLine[1];
		else if	(vTextLine[0] == "Session")							m_stResPacket.stGrlHdr.ghSession =							vTextLine[1];
		else if	(vTextLine[0] == "Speed")								m_stResPacket.stGrlHdr.ghSpeed =							vTextLine[1];
		else if	(vTextLine[0] == "Transport")						m_stResPacket.stGrlHdr.ghTransport =						vTextLine[1];
		//响应报头
		else if	(vTextLine[0] == "Allow")								m_stResPacket.stResHdr.rhAllow =								vTextLine[1];
		else if (vTextLine[0] == "Content-Type")				m_stResPacket.stResHdr.rhContentType =				vTextLine[1];
		else if (vTextLine[0] == "Public")								m_stResPacket.stResHdr.rhPublic =							vTextLine[1];
		else if (vTextLine[0] == "Range")								m_stResPacket.stResHdr.rhRange =							vTextLine[1];
		else if (vTextLine[0] == "Retry-After")						m_stResPacket.stResHdr.rhRetryAfter =					vTextLine[1];
		else if (vTextLine[0] == "RTP-Info")							m_stResPacket.stResHdr.rhRTPInfo =							vTextLine[1];
		else if (vTextLine[0] == "Server")								m_stResPacket.stResHdr.rhServer =							vTextLine[1];
		else if (vTextLine[0] == "Unsupported")					m_stResPacket.stResHdr.rhUnsupported =				vTextLine[1];
		else if (vTextLine[0] == "WWWAuthenticate")		m_stResPacket.stResHdr.rhWWWAuthenticate= 	vTextLine[1];
		//实体报头
		else if (vTextLine[0] == "Content-Base")					m_stResPacket.stEntHdr.ehContentBase =				vTextLine[1];
		else if (vTextLine[0] == "Content-Encoding")		m_stResPacket.stEntHdr.ehContentEncoding =		vTextLine[1];
		else if (vTextLine[0] == "Content-Language")		m_stResPacket.stEntHdr.ehContentLanguage =		vTextLine[1];
		else if (vTextLine[0] == "Content-Length")			m_stResPacket.stEntHdr.ehContentLength =			vTextLine[1];
		else if (vTextLine[0] == "Content-Location")			m_stResPacket.stEntHdr.ehContentLocation =			vTextLine[1];
		else if (vTextLine[0] == "Expires")							m_stResPacket.stEntHdr.ehExpires =							vTextLine[1];
		else if (vTextLine[0] == "Last-Modified")				m_stResPacket.stEntHdr.ehLastModified =				vTextLine[1];
		else ;		//忽略扩展头部
	}

	m_stResPacket.strResEntity = "";
	for (; iter != vTextLineList.end(); iter++)
		m_stResPacket.strResEntity += (*iter)+"\r\n";
}

std::map<std::string, int> RZRTSPAgent::GetRTPFirstSeq() const
{
	std::map<std::string, int> mResult;
	std::vector<std::string> vFields, vUrl, vDomain;
	std::string strSeq;
	bool bFindUrl, bFindSeq;
	int iLength = 0;		//获取域名的长度

	std::vector<std::string> vRTPInfo = RZStream::StreamSplit(m_stResPacket.stResHdr.rhRTPInfo, ",");
	for (std::vector<std::string>::const_iterator iter = vRTPInfo.begin(); iter != vRTPInfo.end(); iter++)
	{
		bFindUrl = bFindSeq = false;
		vFields = RZStream::StreamSplit(*iter, ";");		//每个域以";"分隔
		for (std::vector<std::string>::const_iterator iter1 = vFields.begin(); iter1 != vFields.end(); iter1++)
		{
			vDomain.clear();
			iLength = ::strchr(iter1->c_str(), '=')-iter1->c_str();		//每个域和它的值以"="分隔
			vDomain.push_back(iter1->substr(0, iLength));
			vDomain.push_back(iter1->substr(iLength+1));
			if (vDomain[0] == "url")
			{
				vUrl = RZStream::StreamSplit(vDomain[1], "/");
				bFindUrl = true;
			}
			if (vDomain[0] == "seq")
			{
				strSeq = vDomain[1];
				bFindSeq = true;
			}
			if (bFindUrl && bFindSeq)
				break;
		}
		mResult.insert(make_pair(vUrl.back(), RZTypeConvert::StrToInt(strSeq, 10)));
	}
	return mResult;
}

std::map<std::string, std::string> RZRTSPAgent::GetResTransFields() const
{
	std::map<std::string, std::string> mFields;
	if (m_stResPacket.stGrlHdr.ghTransport == "")
		return mFields;

	std::vector<std::string> vAttriList;
	std::vector<std::string> vTextLineList = RZStream::StreamSplit(m_stResPacket.stGrlHdr.ghTransport, ";");
	for (std::vector<std::string>::const_iterator iter = vTextLineList.begin();
		iter != vTextLineList.end(); iter++)
	{
		vAttriList = RZStream::StreamSplit(*iter, "=");		//server_port与值以"="分隔
		if (vAttriList.size() == 2)
			mFields.insert(make_pair(vAttriList[0], vAttriList[1]));
	}
	return mFields;
}

void RZRTSPAgent::OnResponseOPTIONS()
{
	if (m_stResPacket.stResHdr.rhServer != "")		//显示Server的名字
		printf("Server Name: %s.\n", m_stResPacket.stResHdr.rhServer.c_str());
}

void RZRTSPAgent::OnResponseDESCRIBE()
{
	if (RZTypeConvert::StrToInt(m_stResPacket.stEntHdr.ehContentLength, 10) != m_stResPacket.strResEntity.length())
		Log::ERR("The format of the entity in response packet is not correct.\n");		//检查Content-Length报头
	if (m_stResPacket.stResHdr.rhContentType == CRTSPDesContType)		//检查Content-Type报头
		m_sdpPacket.SetTypeValue(m_stResPacket.strResEntity);		//使用的是sdp协议
	else ;		//使用其他协议，暂时不做任何处理
}

void RZRTSPAgent::OnResponseSETUP(int& iSETUP)
{
	if (m_stResPacket.stGrlHdr.ghSession != "")
	{
		std::vector<std::string> vTextLine = RZStream::StreamSplit(m_stResPacket.stGrlHdr.ghSession, ";");
		m_stHdrCache.strSession = vTextLine[0];
	}

	std::map<std::string, std::string> mFields = GetResTransFields();
	std::string strServerPort, strSSRC;
	std::map<std::string, std::string>::const_iterator iter;
	if ((iter = mFields.find("server_port")) != mFields.end())
	{
		strServerPort = iter->second;
		if (strServerPort != "")
			m_pRTPAgent[iSETUP].SetServerIPAndPort(m_pNetConn->GetPeerIP(), strServerPort);
	}
	iSETUP++;
}

void RZRTSPAgent::OnResponsePLAY()
{
	if (m_stResPacket.stResHdr.rhRange != "")
		printf("Range: %s.\n", m_stResPacket.stResHdr.rhRange.c_str());

	std::map<std::string, int> mSeq = GetRTPFirstSeq();
	if (m_eTask == ENUM_ACCEPTAS || m_eTask == ENUM_ACCEPTVS)
	{
		if (m_eTask == ENUM_ACCEPTAS)
		{
			m_pRTPAgent->SetFirstSeq(mSeq[m_sdpPacket.GetAudioID()]);
			m_pRTPAgent->m_stMediaInfo.iSampFreq = m_sdpPacket.GetSampFrequence(ENUM_AUDIO);
		}
		else		//m_eTask == ENUM_ACCEPTVS
		{
			m_pRTPAgent->SetFirstSeq(mSeq[m_sdpPacket.GetVideoID()]);
			m_pRTPAgent->m_stMediaInfo.iSampFreq = m_sdpPacket.GetSampFrequence(ENUM_VIDEO);
		}
		m_pRTPAgent->StartThread();
		while (!m_pRTPAgent->PackRecvComplete());
	}
	else
	{
		m_pRTPAgent[0].SetFirstSeq(mSeq[m_sdpPacket.GetAudioID()]);
		m_pRTPAgent[1].SetFirstSeq(mSeq[m_sdpPacket.GetVideoID()]);
		m_pRTPAgent[0].m_stMediaInfo.iSampFreq = m_sdpPacket.GetSampFrequence(ENUM_AUDIO);
		m_pRTPAgent[1].m_stMediaInfo.iSampFreq = m_sdpPacket.GetSampFrequence(ENUM_VIDEO);
		m_pRTPAgent[0].StartThread();
	    m_pRTPAgent[1].StartThread();
		while (!m_pRTPAgent[0].PackRecvComplete() || !m_pRTPAgent[1].PackRecvComplete());
	}
}

void RZRTSPAgent::OnResponseTEARDOWN()
{
	if (m_stResPacket.stGrlHdr.ghConnection != "")
		printf("Current connection status is %s.\n", m_stResPacket.stGrlHdr.ghConnection);
}

void RZRTSPAgent::OnDispatchAcceptAVStream()
{
	int iSETUP = 0;
	std::vector<RTSP_METHOD> vTskMethodList = GetTaskMethodList();		//获取方法列表
	if (m_eTask == ENUM_ACCEPTAS)
		m_pRTPAgent = new RZRTPAgent(ENUM_AUDIO);
	else if (m_eTask == ENUM_ACCEPTVS)
		m_pRTPAgent = new RZRTPAgent(ENUM_VIDEO);
	else if (m_eTask == ENUM_ACCEPTAVS)
	{
		m_pRTPAgent = new RZRTPAgent[2];				//0接收音频流，1接收视频流
		m_pRTPAgent[0].SetMediaType(ENUM_AUDIO);
		m_pRTPAgent[1].SetMediaType(ENUM_VIDEO);
	}

	for (std::vector<RTSP_METHOD>::const_iterator iter = vTskMethodList.begin();
				iter != vTskMethodList.end(); iter++) 
	{
		SendRequest(*iter, iSETUP);
		RecvPeerData();
		if (GetCSeq() != m_stHdrCache.iCseq)		//检查CSeq报头
			Log::ERR("Receive an unvalid Cseq.\n");
		if (GetStatusCode() != "200")
			Log::ERR("Get status code from server is %s, not equal to 200.\n", GetStatusCode().c_str());

		switch (*iter)		//此时响应正常
		{
		case ENUM_OPTIONS:			OnResponseOPTIONS();				break;
		case ENUM_DESCRIBE:			OnResponseDESCRIBE();				break;
		case ENUM_SETUP:					OnResponseSETUP(iSETUP);		break;
		case ENUM_PLAY:					OnResponsePLAY();						break;
		case ENUM_TEARDOWN:		OnResponseTEARDOWN();		break;
		}
	}
	printf("The task has been completed! Please check the file in current directory.\n");
	if (m_eTask == ENUM_ACCEPTAS || m_eTask == ENUM_ACCEPTVS)
		delete m_pRTPAgent;
	else if (m_eTask == ENUM_ACCEPTAVS)
		delete []m_pRTPAgent;
}