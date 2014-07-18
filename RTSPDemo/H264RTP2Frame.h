//H264RTP2Frame.h
#ifndef __H264RTP2FRAME_H__
#define __H264RTP2FRAME_H__
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "NETEC/XListPtr.h"
#include "NETEC/XAutoLock.h"

#ifdef RTP_VERSION
#undef RTP_VERSION
#endif
#define RTP_VERSION			0x02

#ifdef RTP_HEADER_LEN
#undef RTP_HEADER_LEN
#endif
#define RTP_HEADER_LEN		0x0C

/// RTP packet header
typedef struct tagRTP_HEADER
{
#if (1)
	unsigned short count:4;              // csrc count
	unsigned short extension:1;          // header extension flag
	unsigned short padding:1;            // padding flag - for encryption
	unsigned short version:2;            // protocol version


	unsigned short type:7;               // payload type
	unsigned short marker:1;             // marker bit - for profile
#else

	unsigned char version:2;             // protocol version
	unsigned char padding:1;             // padding flag - for encryption
	unsigned char extension:1;           // header extension flag
	unsigned char count:4;               // csrc count

	unsigned char marker:1;              // marker bit - for profile
	unsigned char type:7;                // payload type


#endif

	unsigned short sequence;          // sequence number of this packet 
	// (needs endian conversion here, 
	// done in the access methods )

	/// timestamp of this packet
	unsigned long timestamp;
	/// source of packet
	unsigned long ssrc;
}RTP_HEADER,*PRTP_HEADER;


class H264RTP2Frame;
class H264RTP2FrameCallback
{
public:
	virtual void OnH264RTP2FrameCallbackFramePacket(H264RTP2Frame*pH264RTP2Frame,void*pPacketData,int nPacketLen,int nKeyFrame){};
};

//RFC3984
class H264RTP2Frame
{
public:
	H264RTP2Frame(H264RTP2FrameCallback&rCallback,int nMaxBitrate=6144);
	virtual~H264RTP2Frame(void);

public:
	virtual int Open(/*int nPayloadType,int nMaxRTPFrameSize,int nHeaderMargin=0*/);
	virtual void Close(void);
	virtual int GetFrame(unsigned char*pFrameBuf,int nBufLen);
	virtual void OnRecvdRTPPacket(void*pPacketData,int nPacketLen);
protected:
	virtual void PacketsToFrame(void);
protected:
	virtual void FlushRTPPackets(void);
	virtual int CalculateFrameSize(void);
	int handle_rtp_packet( unsigned char*pPayloadData,int nPayloadLen,unsigned char*pFrameBuf,int nBufLen);
	int get_codec_header_len(void);
protected:
	int						m_nPayloadType;
	int						m_nMaxRTPFrameSize;
	int						m_nMaxRTPPayloadBufLen;
	unsigned long			m_nRTPTimestamp;
	unsigned long			m_ulSSRC;
	unsigned short			m_usSeqNo; 

	H264RTP2FrameCallback&	m_rCallback;

	unsigned char*			m_pFrameBuf;
	int						m_nFrameBufLen;

	int						m_nHeaderMargin;

	unsigned long			m_nLastRecvdRTPTime;
	bool					m_bGotFrame;
	int						m_nKeyFrame;

	int						m_nMaxBitrate;
protected:
	class RTPFrame
	{
	public:
		RTPFrame(void)
			: pBuffer(NULL)
			, nLen(0)
			, bFirstPacket(false)
		{
		}
		virtual~RTPFrame(void)
		{
			if(pBuffer)
			{
				free(pBuffer);
				pBuffer=NULL;
			}
		}
	public:
		void Release(void)
		{
			if(pBuffer)
			{
				free(pBuffer);
				pBuffer=NULL;
			}

			delete this;
		}

		void*	pBuffer;
		int		nLen;
		bool	bFirstPacket;
	};

	XListPtr		m_ListRTPPacket;
	XCritSec		m_csListRTPPacket;
	int				m_nBufferedFrameCount;
	int				m_nMaxPacketsToSend;
	unsigned long	m_nLastSentPacketTimestamp;
};

#endif