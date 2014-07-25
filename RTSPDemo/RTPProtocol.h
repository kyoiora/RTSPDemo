#ifndef	RTPPROTOCOL_H_
#define	RTPPROTOCOL_H_

#include <queue>
#include <stdio.h>

#include "MetaObject.h"
#include "NetConn.h"
#include "NetBase.h"
#include "ConsoleText.h"
#include "Logger.h"
#include "H264RTP2Frame.h"

#pragma pack(1)		//文件作用域内的所有结构体字段不对齐也不填充

const unsigned long CSysRecvBufSize = 1024*1024;
const unsigned long CDaySeconds = 24*60*60UL;
const unsigned long CMilli2PicoSec = 1000*1000*1000UL;

class RZRTPAgent;
class RZRTCPAgent;

typedef union
{
	struct
	{
		unsigned char RC : 5;		//接收报告计数，这个值表明当前包中接收报告块（reception report blocks）的数量
		unsigned char P : 1;			//填充位，若置位表示有填充，包的最后一个字节指示填充的字节数（包含其自身）
		unsigned char V : 2;			//指明RTP协议的版本，当前版本号为2
	} bField;
	unsigned char theWholeByte;
} RTCP_FirstByte;

struct _RTCP_SenderInfo
{
	char NTPTimeStamp[8];
	unsigned long RTPTimeStamp;			//与NTPTimeStamp字符串对应的时间表示
	unsigned long PacketCount;				//从开始传输直到生成这个RTCP包期间传输的RTP数据包的总数
	unsigned long PayloadOctets;			//从开始传输直到生成这个RTCP包期间传输的RTP包中有效载荷的总数

	_RTCP_SenderInfo()
	{
		::memset(this, 0, sizeof(_RTCP_SenderInfo));
	}
};
typedef struct _RTCP_SenderInfo		RTCP_SenderInfo;

struct _RTCP_ReportBlocks
{
	unsigned long SSRC_n;				//指明本报告块所属源的同步源标识符
	unsigned char FractionLost;		//指明从上一个SR/RR包发出以来RTP数据包的丢失率（=FractionLost/256）
	char CumuLostPackets[3];			//从开始接收时累计丢失的RTP数据包的总数
	unsigned long ExtHiestSeqNum;			//低16位包含RTP数据包中最大的序列号
	unsigned long InterarrivalJitter;			//接收抖动—RTP数据包接收时间的统计方差估计
	unsigned long LSR;				//上次SR时间戳
	unsigned long DLSR;			//上次发出SR包到本次发送之间的时延

	_RTCP_ReportBlocks()
	{
		::memset(this, 0, sizeof(_RTCP_ReportBlocks));
	}
};
typedef struct _RTCP_ReportBlocks		RTCP_ReportBlocks;

struct _RTCP_SRHeader
{
	RTCP_FirstByte uFirstByte;
	unsigned char PT;				//指明头部类型，例如当这个值为200时指明当前头部是SR类型的头部
	unsigned short length;		//表示头部的长度（=(length+1)*4 bytes），并且包含填充
	unsigned long SSRC;			//指明当前头部的同步源标识符，这个域与RTP头部中的SSRC相同
	RTCP_SenderInfo stSendInfo;
	std::vector<RTCP_ReportBlocks> vReportBlks;

	_RTCP_SRHeader()
		:PT(200),
		length(0),
		SSRC(0),
		stSendInfo(),
		vReportBlks()
	{
		uFirstByte.theWholeByte = 0;
	}
};
typedef struct _RTCP_SRHeader		RTCP_SRHeader;

struct _RTCP_RRHeader
{
	RTCP_FirstByte uFirstByte;		//the same as that of the SR packet except that PT field contains the constant 201
	unsigned char PT;					//& RTCP_SenderInfo structure is omitted
	unsigned short length;
	unsigned long SSRC;
	RTCP_ReportBlocks stReportBlks;

	_RTCP_RRHeader()
		:PT(201),
		stReportBlks()
	{
	}
};
typedef struct _RTCP_RRHeader		RTCP_RRHeader;

struct _RTCP_SDESHeader
{
	RTCP_FirstByte uFirstByte;
	unsigned char PT;
	unsigned short length;
	char chunk[24];

	_RTCP_SDESHeader()
		:PT(202)
	{
	}
};
typedef struct _RTCP_SDESHeader		RTCP_SDESHeader;

struct _RTCP_BYEHeader
{
	RTCP_FirstByte uFirstByte;
	unsigned char PT;
	unsigned short length;
	unsigned long SSRC;

	_RTCP_BYEHeader()
		:PT(203)
	{
	}
};
typedef struct _RTCP_BYEHeader			RTCP_BYEHeader;

typedef enum
{
	ENUM_RTCP_SR			= 200,			//Sender Report—发送端报告
	ENUM_RTCP_RR			= 201,			//Receiver Report—接收端报告
	ENUM_RTCP_SDES		= 202,			//Source Description Items—源点描述
	ENUM_RTCP_BYE			= 203,			//结束传输
	ENUM_RTCP_APP			= 204			//特定应用
} RTCP_HEADER_TYPE;

struct _RTCP_RecvPacket
{
	RTCP_SRHeader stSRHdr;
	//... 可以添加更多类型的头部

	unsigned long SSRC;
	bool bGetLastPack;			//是否已取得最后一个RTCP包
	unsigned long ulTimeStamp;		//接收到最新RTCP包的时间戳
	unsigned long ulLSRPackCnt;		//前一个RTCP包中推送的RTP包的已发总数

	_RTCP_RecvPacket()
		:stSRHdr(),
		SSRC(0),
		bGetLastPack(false),
		ulTimeStamp(0),
		ulLSRPackCnt(0)
	{
	}
};
typedef struct _RTCP_RecvPacket		RTCP_RecvPacket;

struct _RTCP_SendPacket
{
	RTCP_RRHeader stRRHdr;
	RTCP_SDESHeader stSDESHdr;
	RTCP_BYEHeader stBYEHdr;

	unsigned long nCumuLost;
	unsigned int uLastJitter;

	_RTCP_SendPacket()
		:stRRHdr(),
		 stSDESHdr(),
		 stBYEHdr(),
		 nCumuLost(0),
		 uLastJitter(0)
	{
	}
};
typedef struct _RTCP_SendPacket		RTCP_SendPacket;

class RZRTCPAgent : public RZAgent
{
public:
	RZRTCPAgent(RZRTPAgent*);
	~RZRTCPAgent() {}

public:
	inline void SetLocalSSRC(unsigned long);

private:
	void ThreadProc();
	void ParsePacket(const char*, unsigned long);
	void OnResponseSR(const char*);
	void OnResponseBYE();
	unsigned long GeneSSRC() const;

	void SetReportBlocks();
	unsigned char CalculateFractionLost();
	unsigned int CalculateInterJitter();
	unsigned long CalculateLSRDelay() const;

private:
	RTCP_RecvPacket m_rPacket;
	RTCP_SendPacket m_sPacket;
	RZRTPAgent* m_pRTPAgent;
};

inline void RZRTCPAgent::SetLocalSSRC(unsigned long _ulSSRC)
{
	m_rPacket.SSRC = _ulSSRC;
}

//////////////////////////////////////////////////////////////////////////
//RZCyclePool类
struct _RangeDetect
{
	unsigned long iFocus;
	unsigned long iBeyondCnts;

	_RangeDetect()
		:iFocus(0),
		 iBeyondCnts(0)
	{
	}
};
typedef struct _RangeDetect		RangeDetect;

class RZCyclePool
{
public:
	RZCyclePool(unsigned long _nCycles = 4);
	~RZCyclePool();

public:
	inline void SetFirstFocus(unsigned long);
	void Insert(unsigned long _index, const char* _pSrc, unsigned long _ulSize);
	RZNetStrPool* GetFocusPool() const;
	void UpdateFocusPool();
	std::vector<RZNetStrPool*> GetValidPool() const;

private:
	RZNetStrPool*	m_pNetStrPool;
	RangeDetect		m_stRngDetect;
	int m_iCycles;
};

inline void RZCyclePool::SetFirstFocus(unsigned long _ulSeqNum)
{
	unsigned long index = _ulSeqNum%(m_iCycles*CPoolSlots);
	m_stRngDetect.iFocus = index/CPoolSlots;
}

//////////////////////////////////////////////////////////////////////////
//RZRTPAgent类
typedef union
{
	struct
	{
		/*结构体的内存模型为：  高位 |-V-|-P-|-X-|-CC-| 低位  */
		unsigned char CC : 4;	//表示固定头部后面跟着的CSRC的数目
		unsigned char X : 1;		//该位置位，头部后面跟有一个扩展头部
		unsigned char P : 1;		//该位置位，RTP包的尾部含有附加的填充字节，填充的最后一个字节指明填充字节数
		unsigned char V : 2;		//表示RTP的版本
	} bField;
	unsigned char theWholeByte;
} RTPFirstByte;

typedef union
{
	struct
	{
		unsigned char PT : 7;		//表示RTP数据流的编码类型
		unsigned char M : 1;		//位的解释由配置文档（profile）来承担
	} bField;
	unsigned char theWholeByte;
} RTPSecByte;

struct _RTPHeader			//不需要解析RTP包的头部，但把定义保留在这里
{
	RTPFirstByte		uFirByte;
	RTPSecByte		uSecByte;
	unsigned short	SeqNum;
	unsigned long TimeStamp;
	unsigned long SSRC;

	_RTPHeader() 
	{
		::memset(this, 0, sizeof(_RTPHeader));
	}
};
typedef struct _RTPHeader	RTPHeader;

struct _MediaInfo
{
	int iFirstSeq;				//第一个包的序列号，存放在RTSP响应包的RTP-Info报头中
	int iPackNum;			//包的总数
	bool bRecvAll;			//包是否已全部接收完毕
	unsigned long ulSSRC;			//包的同步源标识符
	int iSampFreq;			//采样频率

	_MediaInfo()
		:iFirstSeq(0),
		 iPackNum(-1),		//初始化为-1，指明还未收到最后一个包
		 bRecvAll(false),
		 ulSSRC(0)
	{
	}
};
typedef struct _MediaInfo			MediaInfo;

struct _LostPackStatis
{
	int iStart, iEnd;		//单个缓冲池的起始与结束位置
	unsigned long nRecvPacks;	//实际收到的包
	unsigned long nHndleCnt;		//已处理的包的总数
	unsigned long uThresHold;	//当前阀值
	std::vector<unsigned long> vLostPack;		//存放丢失的包的序列号（转换后唯一）

	unsigned long ulFirSeqWallClck;		//RTCP携带的第一个墙上时钟，单位为毫秒
	unsigned long ulFirSeqRTPStmp;		//对应的第一个RTP时间戳
	std::vector<unsigned long> vPackDiff;		//the difference D in packet
	RZSemaphore mSemaphore;

	_LostPackStatis()
		:iStart(0),
		 iEnd(0),
		 nRecvPacks(0),
		 nHndleCnt(0),
		 uThresHold(0),
		 vLostPack(),
		 ulFirSeqWallClck(0),
		 ulFirSeqRTPStmp(0),
		 mSemaphore(1, 1)
	{
		vPackDiff.push_back(0);
	}
};
typedef struct _LostPackStatis		LostPackStatis;

class RZRTPAgent
	:public RZAgent,
	 public H264RTP2FrameCallback
{
	//declaration
	friend class RZRTSPAgent;
	friend class RZRTCPAgent;

public:
	RZRTPAgent(STREAM_MEDIA_TYPE _eMediaType = ENUM_MEDIA_UNK);
	~RZRTPAgent();

public:
	inline RZNetPort GetLocalPort() const;
	inline void SetFirstSeq(int);
	inline void SetSSRC(unsigned long);
	inline unsigned long GetHiestSeqNum() const;
	inline bool PackRecvComplete() const;
	void SetMediaType(STREAM_MEDIA_TYPE _eMediaType);
	void SetServerIPAndPort(const RZNetIPAddr&, const std::string&);

private:
	void InitListenPort();
	void ThreadProc();
	void ParsePacket(const char*, unsigned long);
	void RemvPackDiff();
	void RemvLostPack();
	void OnSinglePool(const RZNetStrPool*);
	void FlushCyclePool();
	inline void WriteMediaFile(void* pPacketData, int nPacketLen);
	virtual void OnH264RTP2FrameCallbackFramePacket(H264RTP2Frame*pH264RTP2Frame,void*pPacketData,int nPacketLen,int nKeyFrame);

private:
	STREAM_MEDIA_TYPE		m_eMediaType;		//当前接收的数据流的类型
	MediaInfo					m_stMediaInfo;
	LostPackStatis			m_stLostPackStatis;		//丢包统计
	RZCyclePool				m_stCyclePool;
	RZRTCPAgent*			m_pRTCPAgent;
	H264RTP2Frame*	m_pRTP2Frame;
	FILE*	m_pFrameFile;
	bool	m_bWantToStop;

	RZConsoleOutput	m_ConsoleOutput;
	static RZSemaphore	m_Semaphore;
};

inline RZNetPort RZRTPAgent::GetLocalPort() const
{
	RZUdpConn* pUdpConn = dynamic_cast<RZUdpConn*>(m_pNetConn);
	return pUdpConn->GetLocalPort();
}

inline bool RZRTPAgent::PackRecvComplete() const
{
	return m_stMediaInfo.bRecvAll;
}

inline void RZRTPAgent::SetFirstSeq(int _iSeq)
{
	m_stMediaInfo.iFirstSeq = _iSeq;
	m_stLostPackStatis.iStart = _iSeq%CPoolSlots;
	m_stCyclePool.SetFirstFocus(_iSeq);
}

inline void RZRTPAgent::SetSSRC(unsigned long _SSRC)
{
	m_stMediaInfo.ulSSRC = _SSRC;
}

inline unsigned long RZRTPAgent::GetHiestSeqNum() const
{
	return (m_stMediaInfo.iFirstSeq+m_stLostPackStatis.nHndleCnt-1);
}

inline void RZRTPAgent::WriteMediaFile(void* pPacketData, int nPacketLen)
{
	::fwrite(pPacketData, sizeof(char), nPacketLen, m_pFrameFile);
}
#endif			//RTPPROTOCOL_H_