#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <algorithm>
#include "RTPProtocol.h"

const std::string CRTPSSRC = "HP StreamMedia";
#ifdef WIN32
const std::string CAudioFrame = "e:\\OutputDir\\AudioFrame.aac";
const std::string CVideoFrame = "e:\\OutputDir\\VideoFrame.h264";
#endif

RZRTCPAgent::RZRTCPAgent(RZRTPAgent* _pRTPAgent)
:RZAgent(new RZUdpConn),
 m_rPacket(),
 m_sPacket(),
 m_pRTPAgent(_pRTPAgent)
{
	unsigned long uSSRC = GeneSSRC();
	//RRHeader
	m_sPacket.stRRHdr.uFirstByte.theWholeByte = 0x81;
	m_sPacket.stRRHdr.length = ::htons(static_cast<unsigned short>((8+24)/4-1));
	m_sPacket.stRRHdr.SSRC = uSSRC;
	//SDESHeader
	m_sPacket.stSDESHdr.uFirstByte.theWholeByte = 0x81;
	m_sPacket.stSDESHdr.length = ::htons(static_cast<unsigned short>(28/4-1));
	::memset(m_sPacket.stSDESHdr.chunk, 0, 24);
	::memcpy(m_sPacket.stSDESHdr.chunk, &uSSRC, sizeof(unsigned long));		//4
	m_sPacket.stSDESHdr.chunk[4] = 0x01;		//4+1
	m_sPacket.stSDESHdr.chunk[5] = CRTPSSRC.length();			//5+1
	::memcpy(m_sPacket.stSDESHdr.chunk+6, CRTPSSRC.c_str(), CRTPSSRC.length());		//6+14
	//BYEHeader
	m_sPacket.stBYEHdr.uFirstByte.theWholeByte = 0x81;
	m_sPacket.stBYEHdr.length = ::htons(static_cast<unsigned short>(1));
	m_sPacket.stBYEHdr.SSRC = uSSRC;
	//在响应对端的RTCP包时只需要填充ReportBlocks就可以了
}

unsigned long RZRTCPAgent::GeneSSRC() const
{
	unsigned long uSSRC;
	for (int i = 0; i < sizeof(unsigned long); i++)
		((char*)&uSSRC)[i] = rand()%256;
	return uSSRC;
}

void RZRTCPAgent::SetReportBlocks()
{
	m_sPacket.stRRHdr.stReportBlks.SSRC_n = ::htonl(m_rPacket.SSRC);		//SSRC_n
	m_sPacket.stRRHdr.stReportBlks.FractionLost = CalculateFractionLost();		//Fraction lost
	unsigned long ulLost = ::htonl(m_sPacket.nCumuLost);		//network byte order
	::memcpy(&m_sPacket.stRRHdr.stReportBlks.CumuLostPackets, ((char*)&ulLost)+1, 3);			//cumulative lost packets
	m_sPacket.stRRHdr.stReportBlks.ExtHiestSeqNum = ::htonl(m_pRTPAgent->GetHiestSeqNum());	//extended highest seq num
	m_sPacket.stRRHdr.stReportBlks.InterarrivalJitter = ::htonl(CalculateInterJitter());			//interarrival jitter
	::memcpy(&m_sPacket.stRRHdr.stReportBlks.LSR, m_rPacket.stSRHdr.stSendInfo.NTPTimeStamp+2, 4);		//LSR
	m_sPacket.stRRHdr.stReportBlks.DLSR = ::htonl(CalculateLSRDelay());		//DLSR
}

unsigned char RZRTCPAgent::CalculateFractionLost()
{
	const LostPackStatis* pStatis = &m_pRTPAgent->m_stLostPackStatis;
	unsigned long nCount, nTotalNum;
	nCount = 0;
	nTotalNum = m_rPacket.stSRHdr.stSendInfo.PacketCount;

	if (pStatis->nHndleCnt < m_rPacket.stSRHdr.stSendInfo.PacketCount)
	{
		RZTime::Sleep(800);
		if (pStatis->nHndleCnt < m_rPacket.stSRHdr.stSendInfo.PacketCount)
		{
			//等待一段时间之后仍处于上述状态，且这段时间已超出RTPAgent的接收超时间隔
			nTotalNum = pStatis->nHndleCnt;
			nCount = m_rPacket.stSRHdr.stSendInfo.PacketCount-pStatis->nHndleCnt;		//计入丢失
		}
	}

	m_pRTPAgent->m_stLostPackStatis.mSemaphore.Wait();
	for(std::vector<unsigned long>::const_iterator iter = pStatis->vLostPack.begin();
			iter != pStatis->vLostPack.end(); iter++)
	{
		if (*iter < m_rPacket.ulLSRPackCnt)
			continue;
		else if (*iter < nTotalNum)		//*iter >= rPacket.ulLSRPackCnt
			nCount++;
		else			//*iter >= nTotalNum
			break;
	}
	m_sPacket.nCumuLost += nCount;
	m_pRTPAgent->m_stLostPackStatis.uThresHold = nTotalNum;
	m_pRTPAgent->m_stLostPackStatis.mSemaphore.Release();
	return static_cast<unsigned char>(1.0f*nCount/(m_rPacket.stSRHdr.stSendInfo.PacketCount-m_rPacket.ulLSRPackCnt)*256);
}

unsigned int RZRTCPAgent::CalculateInterJitter()
{
	unsigned int uJitter = m_sPacket.uLastJitter;
	if (m_pRTPAgent->m_stLostPackStatis.vPackDiff.size() == 1)
		RZTime::Sleep(200);

	if (m_pRTPAgent->m_stLostPackStatis.vPackDiff.size() > 1)
	{
		unsigned int uPackDiff;
		m_pRTPAgent->m_stLostPackStatis.mSemaphore.Wait();
		for (std::vector<unsigned long>::const_iterator iter = m_pRTPAgent->m_stLostPackStatis.vPackDiff.begin()+1;
				iter != m_pRTPAgent->m_stLostPackStatis.vPackDiff.end(); iter++)
		{
			uPackDiff = ::abs(*iter-*(iter-1));
			uJitter = (uJitter*15+uPackDiff*1)>>4;		//J(i)=J(i-1)*15/16+|D(i-1, i)|*1/16
		}
		m_pRTPAgent->m_stLostPackStatis.vPackDiff.erase(m_pRTPAgent->m_stLostPackStatis.vPackDiff.begin(),
																							m_pRTPAgent->m_stLostPackStatis.vPackDiff.end()-1);
		m_pRTPAgent->m_stLostPackStatis.mSemaphore.Release();
		m_sPacket.uLastJitter = uJitter;
	}
	return uJitter;
}

unsigned long RZRTCPAgent::CalculateLSRDelay() const
{
	unsigned long ulTimeInter = RZTime::GetTimeStamp()-m_rPacket.ulTimeStamp;		//单位为毫秒
	return static_cast<unsigned long>(ulTimeInter*65536/1000.0);		//以1/65536 seconds为单位
}

void RZRTCPAgent::ParsePacket(const char* _pBuffer, unsigned long _ulSize)
{
	if (_ulSize < 4)
		return;

	unsigned short uType = static_cast<unsigned char>(_pBuffer[1]);
	if (uType > 204 || uType < 200)
		return;		//头部类型未知
	else
		m_rPacket.ulTimeStamp = RZTime::GetTimeStamp();

	const char* pValidStr = _pBuffer;
	unsigned short uLength = 0;
	while (pValidStr != _pBuffer+_ulSize)
	{	
		uType = static_cast<unsigned char>(pValidStr[1]);
		uLength = ::ntohs(*(unsigned short*)(&pValidStr[2]));
		switch (uType)
		{
		case ENUM_RTCP_SR:		OnResponseSR(pValidStr);	break;
		case ENUM_RTCP_BYE:	OnResponseBYE();					break;
		case ENUM_RTCP_RR:
		case ENUM_RTCP_SDES:
		case ENUM_RTCP_APP:
		default:		break;
		}
		pValidStr += (uLength+1)*4;		//skip the header has been parsed!
	}
}

void RZRTCPAgent::OnResponseSR(const char* _pValidStr)
{
	if (m_pRTPAgent->m_stLostPackStatis.ulFirSeqWallClck == 0)		//还未设置第一个墙上时钟
	{
		unsigned long NTP_MSW	= ::ntohl(*(unsigned long*)(&_pValidStr[8]));			//8+4
		unsigned long NTP_LSW	= ::ntohl(*(unsigned long*)(&_pValidStr[12]));		//12+4
		unsigned long ulSeconds = NTP_MSW&CDaySeconds;
		m_pRTPAgent->m_stLostPackStatis.ulFirSeqRTPStmp = ::ntohl(*(unsigned long*)(&_pValidStr[16]));
		m_pRTPAgent->m_stLostPackStatis.ulFirSeqWallClck = ulSeconds*1000+NTP_LSW/CMilli2PicoSec;
		RZTime::Sleep(100);		//睡眠少量时间，等待RTP线程处理
	}

	m_rPacket.SSRC = ::ntohl(*(unsigned long*)(&_pValidStr[4]));
	::memcpy(m_rPacket.stSRHdr.stSendInfo.NTPTimeStamp, _pValidStr+8, 8);
	if (m_rPacket.stSRHdr.stSendInfo.PacketCount != 0)		//不是第一个RTCP包
		m_rPacket.ulLSRPackCnt = m_rPacket.stSRHdr.stSendInfo.PacketCount;
	m_rPacket.stSRHdr.stSendInfo.PacketCount = ::ntohl(*(unsigned long*)(&_pValidStr[20]));			//20+4
	m_rPacket.stSRHdr.stSendInfo.PayloadOctets = ::ntohl(*(unsigned long*)(&_pValidStr[24]));		//24+4
}

void RZRTCPAgent::OnResponseBYE()
{
	m_rPacket.bGetLastPack = true;
	m_pRTPAgent->m_stMediaInfo.iPackNum = m_rPacket.stSRHdr.stSendInfo.PacketCount;
}

void RZRTCPAgent::ThreadProc()
{
	printf("[Thread] RTCPAgent has start successfully, it is receiving the QoS ...\n");
	m_pNetConn->SetSysRecvBufSize(CSysRecvBufSize);
	int iSize = sizeof(RTCP_RRHeader)+sizeof(RTCP_SDESHeader);

	while (!m_rPacket.bGetLastPack)
	{
		RecvPeerData();			//接收数据并解析
		SetReportBlocks();		//设置报告块
		if (m_rPacket.bGetLastPack)
			iSize += sizeof(RTCP_BYEHeader);
		m_pNetConn->SendDataToPeer((char*)&m_sPacket, iSize);		//发送接收报告
	}
	printf("[Thread] RTCPAgent has received the last packet, exit normally.\n");
}

//////////////////////////////////////////////////////////////////////////
//RZCyclePool类
RZCyclePool::RZCyclePool(unsigned long _nCycles /* = 4 */)
:m_iCycles(_nCycles),
 m_stRngDetect()
{
	m_pNetStrPool = new RZNetStrPool[m_iCycles];
	if (m_pNetStrPool == NULL)
		Log::ERR("New RZNetStrPool failed.\n");
}

RZCyclePool::~RZCyclePool()
{
	if (m_pNetStrPool != NULL)
	{
		delete []m_pNetStrPool;
		m_pNetStrPool = NULL;
	}
}

void RZCyclePool::Insert(unsigned long _index, const char* _pSrc, unsigned long _ulSize)
{
	unsigned long index = _index%(m_iCycles*CPoolSlots);
	int iPos = index/CPoolSlots;
	if (m_stRngDetect.iFocus != iPos)
		m_stRngDetect.iBeyondCnts++;
	//不处理迟到的包和超前太多时间到达的包
	if (iPos == m_stRngDetect.iFocus || iPos == (m_stRngDetect.iFocus+1)%m_iCycles)
		m_pNetStrPool[iPos].Insert(index%CPoolSlots, _pSrc, _ulSize);
}

RZNetStrPool* RZCyclePool::GetFocusPool() const
{
	if (m_stRngDetect.iBeyondCnts == 16)		//最多等待16个包的时间，以数据驱动而非时间驱动
		return &m_pNetStrPool[m_stRngDetect.iFocus];
	else
		return NULL;
}

void RZCyclePool::UpdateFocusPool()
{
	m_pNetStrPool[m_stRngDetect.iFocus].Clear();
	m_stRngDetect.iFocus = (m_stRngDetect.iFocus+1)%m_iCycles;
	m_stRngDetect.iBeyondCnts = 0;
}

std::vector<RZNetStrPool*> RZCyclePool::GetValidPool() const
{
	std::vector<RZNetStrPool*> vPoolList;
	unsigned long index;
	for (int i = 0; i < m_iCycles; i++)
	{
		index = (m_stRngDetect.iFocus+i)%m_iCycles;
		if (m_pNetStrPool[index].GetItems() != 0)
			vPoolList.push_back(&m_pNetStrPool[index]);
		else
			break;		//有效缓冲池必定连续
	}
	return vPoolList;
}

//////////////////////////////////////////////////////////////////////////
//RZRTPAgent类
RZRTPAgent::RZRTPAgent(STREAM_MEDIA_TYPE _eMediaType /* = ENUM_MEDIA_UNK */)
:RZAgent(new RZUdpConn),
 m_eMediaType(_eMediaType),
 m_stMediaInfo(),
 m_stCyclePool(),
 m_stLostPackStatis(),
 m_RTCPAgent(this),
 m_pFrameFile(NULL),
 m_hRTP2Frame(*this),
 m_bWantToStop(false)
{
	InitListenPort();
	if (m_eMediaType != ENUM_MEDIA_UNK)
	{
		const char* pFileName = (m_eMediaType == ENUM_AUDIO)?CAudioFrame.c_str():CVideoFrame.c_str();
		m_pFrameFile = ::fopen(pFileName, "wb");
		if (m_pFrameFile == NULL)
			Log::ERR("C Standard Library \'fopen\' called failed.\n");
	}
	m_stLostPackStatis.iEnd = CPoolSlots;
	m_hRTP2Frame.Open();
}

RZRTPAgent::~RZRTPAgent()
{
	if (m_pFrameFile != NULL)
	{
		::fclose(m_pFrameFile);
		m_pFrameFile = NULL;
	}
}

void RZRTPAgent::InitListenPort()
{
	RZNetPort nRTPPort(ENUM_UDP), nRTCPPort(ENUM_UDP);
	do 
	{
		nRTPPort.RandSelectValidPort(ENUM_DYNAMIC);
		nRTCPPort.SetPortValue(nRTPPort.GetPortValue()+1);		//使用连续的2个端口
	} while (!nRTCPPort.PortValid());

	this->SetLocalPort(nRTPPort);
	m_RTCPAgent.SetLocalPort(nRTCPPort);
}

void RZRTPAgent::SetMediaType(STREAM_MEDIA_TYPE _eMediaType)
{
	if (_eMediaType == ENUM_MEDIA_UNK)
		Log::ERR("Unvalid media type, please set correctly.\n");

	m_eMediaType = _eMediaType;
	if (m_pFrameFile != NULL)
		::fclose(m_pFrameFile);
	if (m_eMediaType == ENUM_AUDIO)
	{
		m_pFrameFile = ::fopen(CAudioFrame.c_str(), "wb");
		if (m_pFrameFile == NULL)
			Log::ERR("C Standard Library \'fopen\' called failed.\n");
	}
	else		//eMediaType == ENUM_VIDEO
	{
		m_pFrameFile = ::fopen(CVideoFrame.c_str(), "wb");
		if (m_pFrameFile == NULL)
			Log::ERR("C Standard Library \'fopen\' called failed.\n");
	}
}

void RZRTPAgent::SetServerIPAndPort(const RZNetIPAddr& _nIPAddr, const std::string& _str)
{
	std::vector<std::string> vPortList = RZStream::StreamSplit(_str, "-");
	RZNetPort nRTPPort(RZTypeConvert::StrToInt(vPortList[0], 10), ENUM_UDP);
	RZNetPort nRTCPPort(RZTypeConvert::StrToInt(vPortList[1], 10), ENUM_UDP);
	this->ConnetToPeer(_nIPAddr, nRTPPort);
	m_RTCPAgent.ConnetToPeer(_nIPAddr, nRTCPPort);
}

void RZRTPAgent::ThreadProc()
{
	printf("[Thread]RTPAgent has start successfully, it is receiving Real-Time packets from server ...\n");
	m_pNetConn->SetSysRecvBufSize(CSysRecvBufSize);
	m_RTCPAgent.StartThread();	

	RZNetStrPool* pNetStrPool = NULL;
	while (!m_bWantToStop)
	{
		RecvPeerData();
		pNetStrPool = m_stCyclePool.GetFocusPool();
		if (pNetStrPool != NULL)
			OnSinglePool(&m_hRTP2Frame, pNetStrPool);
		if (m_stLostPackStatis.uThresHold != 0)
			RemvLostPack();
	}
	FlushCyclePool(&m_hRTP2Frame);
	m_stMediaInfo.bRecvAll = true;
	m_hRTP2Frame.Close();
	printf("[Thread] Receive Real-Time Stream File Complete! Exit normally.\n");
}

void RZRTPAgent::ParsePacket(const char* _pBuffer, unsigned long _ulSize)
{
	if (_ulSize < 12)			//RTP头部至少有12个字节，所以一个udp数据包应>=12
		return;

	RTPHeader stRTPHdr;
	stRTPHdr.uFirByte.theWholeByte = _pBuffer[0];		//1
	stRTPHdr.uSecByte.theWholeByte = _pBuffer[1];		//1+1
	stRTPHdr.SeqNum = ::ntohs(*(unsigned short*)(&_pBuffer[2]));		//2+2
	stRTPHdr.TimeStamp = ::ntohl(*(unsigned long*)(&_pBuffer[4]));	//4+4
	stRTPHdr.SSRC = ::ntohl(*(unsigned long*)(&_pBuffer[8]));		//8+4
	if (stRTPHdr.uFirByte.bField.V == 2 /*&& stRTPHdr.SSRC == stMediaInfo.ulSSRC*/)
	{
		//检查RTP协议的版本和包的同步源标识符，并且没有去头部和填充
		m_stCyclePool.Insert(stRTPHdr.SeqNum, _pBuffer, _ulSize);
		if (m_stLostPackStatis.ulFirSeqWallClck != 0)		//由RTCP设置
		{
			unsigned long uClckTmDiff = RZTime::GetWallClockTime()-m_stLostPackStatis.ulFirSeqWallClck;		//ms
			unsigned long uSampDiff = static_cast<unsigned long>(uClckTmDiff/1000.0f*m_stMediaInfo.iSampFreq);
			m_stLostPackStatis.mSemaphore.Wait();
			m_stLostPackStatis.vPackDiff.push_back(uSampDiff+m_stLostPackStatis.ulFirSeqRTPStmp-stRTPHdr.TimeStamp);
			m_stLostPackStatis.mSemaphore.Release();
		}
	}
}

void RZRTPAgent::RemvLostPack()
{
	m_stLostPackStatis.mSemaphore.Wait();			//为了防止RTCP中的迭代器失效，还是要加锁
	for (std::vector<unsigned long>::const_iterator iter = m_stLostPackStatis.vLostPack.begin();
			iter != m_stLostPackStatis.vLostPack.end();)
	{
		if (*iter < m_stLostPackStatis.uThresHold)
			iter = m_stLostPackStatis.vLostPack.erase(iter);
		else
			iter++;
	}
	m_stLostPackStatis.mSemaphore.Release();
}

void RZRTPAgent::OnSinglePool(H264RTP2Frame* _pRTP2Frame, const RZNetStrPool* _pNetStrPool)
{
	BufferPool stLocalPool = _pNetStrPool->GetBufferPool();
	m_stLostPackStatis.mSemaphore.Wait();
	for (unsigned long i = m_stLostPackStatis.iStart; i != m_stLostPackStatis.iEnd; i++)
	{
		if (stLocalPool.pSize[i] != 0 && _pRTP2Frame)
			_pRTP2Frame->OnRecvdRTPPacket(stLocalPool.pBufferPool+i*CPoolSlotSize, 
																			stLocalPool.pSize[i]);
		else			//stLocalPool.pSize[i] == 0，丢包
			m_stLostPackStatis.vLostPack.push_back(m_stLostPackStatis.nHndleCnt);
		m_stLostPackStatis.nHndleCnt++;
	}
	m_stCyclePool.UpdateFocusPool();
	m_stLostPackStatis.mSemaphore.Release();
	m_stLostPackStatis.iStart = 0;
}

void RZRTPAgent::OnH264RTP2FrameCallbackFramePacket(H264RTP2Frame*pH264RTP2Frame,void*pPacketData,int nPacketLen,int nKeyFrame)
{
	::fwrite(pPacketData, sizeof(char), nPacketLen, m_pFrameFile);
}

void RZRTPAgent::FlushCyclePool(H264RTP2Frame* _pRTP2Frame)
{
	std::vector<RZNetStrPool*> vPoolList = m_stCyclePool.GetValidPool();
	for (std::vector<RZNetStrPool*>::const_iterator iter = vPoolList.begin();
			iter != vPoolList.end(); iter++)
	{
		if (iter == vPoolList.begin()+vPoolList.size()-1)		//最后一个有效的缓冲池
		{
			unsigned long nValidItems = (*iter)->GetItems();
			int index = 0;
			while(nValidItems)
			{
				if ((*iter)->GetBufferPool().pSize[index++] != 0)
					nValidItems--;
			}
			m_stLostPackStatis.iEnd = index;
		}
		OnSinglePool(_pRTP2Frame, *iter);
	}
}