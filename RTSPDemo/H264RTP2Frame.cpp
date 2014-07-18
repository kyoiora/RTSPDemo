//#include "stdafx.h"
#include "H264RTP2Frame.h"
//#include "NETEC/XSocket.h"
#include "NETEC/XUtil.h"

//#ifdef WIN32
//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif
//#endif
#pragma comment(lib,"NETEC.lib")
H264RTP2Frame::H264RTP2Frame(H264RTP2FrameCallback&rCallback,int nMaxBitrate)
:m_nPayloadType(34)
,m_nMaxRTPFrameSize(1400)
,m_nMaxRTPPayloadBufLen(m_nMaxRTPFrameSize-RTP_HEADER_LEN)

,m_nRTPTimestamp(0)
,m_ulSSRC(XGenerateSSRC())
,m_usSeqNo(0)

,m_rCallback(rCallback)
,m_nKeyFrame(0)
,m_nMaxBitrate(nMaxBitrate+(nMaxBitrate>>1))
{
	m_pFrameBuf=NULL;
	m_nFrameBufLen=0;

	m_nHeaderMargin=0;

	m_nLastRecvdRTPTime=0;
	m_bGotFrame=false;

	m_nBufferedFrameCount=0;
	m_nMaxPacketsToSend=3;
	m_nLastSentPacketTimestamp=0;
}

H264RTP2Frame::~H264RTP2Frame(void)
{

}

int H264RTP2Frame::Open(/*int nPayloadType,int nMaxRTPFrameSize,int nHeaderMargin*/)
{/*
	m_nHeaderMargin=nHeaderMargin;
	m_nPayloadType=nPayloadType;
	m_nMaxRTPFrameSize=nMaxRTPFrameSize;*/
	m_nMaxRTPPayloadBufLen=m_nMaxRTPFrameSize-RTP_HEADER_LEN-get_codec_header_len();
	return 0;
}

void H264RTP2Frame::Close(void)
{
	FlushRTPPackets();

	if (m_pFrameBuf!=NULL)
	{
		free(m_pFrameBuf);
		m_pFrameBuf=NULL;
	}
	m_nFrameBufLen=0;
}

void H264RTP2Frame::FlushRTPPackets(void)
{
	XAutoLock l(m_csListRTPPacket);
	while (m_ListRTPPacket.size()>0)
	{
		RTPFrame*pRTPFrame=(RTPFrame*)m_ListRTPPacket.front();
		m_ListRTPPacket.pop_front();
		pRTPFrame->Release();
		pRTPFrame=NULL;
	}
}


int H264RTP2Frame::CalculateFrameSize(void)
{
	int nFrameLen=0;
	XAutoLock l(m_csListRTPPacket);
	XListPtr::iterator iter=m_ListRTPPacket.begin();
	while (iter!=m_ListRTPPacket.end())
	{
		RTPFrame*pRTPFrame=(RTPFrame*)*iter;
		nFrameLen+=pRTPFrame->nLen;
		++iter;
	}
	return nFrameLen;
}

int H264RTP2Frame::GetFrame(unsigned char*pFrameBuf,int nBufLen)
{
	int nFrameLen=0;
	XAutoLock l(m_csListRTPPacket);
	while (m_ListRTPPacket.size()>0)
	{
		RTPFrame*pRTPFrame=(RTPFrame*)m_ListRTPPacket.front();
		m_ListRTPPacket.pop_front();

		int nTemp=handle_rtp_packet((unsigned char*)pRTPFrame->pBuffer+RTP_HEADER_LEN,pRTPFrame->nLen-RTP_HEADER_LEN,pFrameBuf,nBufLen);
		nFrameLen+=nTemp;

		pFrameBuf+=nTemp;
		nBufLen-=nBufLen;

		pRTPFrame->Release();
		pRTPFrame=NULL;
	}

	return nFrameLen;
}

void H264RTP2Frame::OnRecvdRTPPacket(void*pPacketData,int nPacketLen)
{
	unsigned char*pFrameData=(unsigned char*)pPacketData;
	if (pFrameData!=NULL && nPacketLen>=12)
	{
		PRTP_HEADER pRTPHeader=(PRTP_HEADER)pFrameData;
		unsigned long nRTPTime=ntohl(pRTPHeader->timestamp);

		//char szDebug[1024];
		//sprintf(szDebug,"nRTPTime=%u m_nLastRecvdRTPTime=%u\n",nRTPTime,m_nLastRecvdRTPTime);
		//OutputDebugString(szDebug);

		if (m_bGotFrame && m_nLastRecvdRTPTime!=nRTPTime)
		{
			PacketsToFrame();
			m_bGotFrame=false;
		}

		RTPFrame*pRTPFrame=new RTPFrame;
		if (pRTPFrame==NULL)
		{
			return;
		}

		pRTPFrame->pBuffer=malloc(nPacketLen);
		if (pRTPFrame->pBuffer!=NULL)
		{
			memcpy(pRTPFrame->pBuffer,pPacketData,nPacketLen);
			pRTPFrame->nLen=nPacketLen;

			XAutoLock l(m_csListRTPPacket);
			m_ListRTPPacket.push_back(pRTPFrame);
		}
		else
		{
			pRTPFrame->Release();
			pRTPFrame=NULL;
			return;
		}

		m_bGotFrame=true;
		m_nLastRecvdRTPTime=nRTPTime;

		if (pRTPHeader->marker)//Marker
		{
			PacketsToFrame();
			m_bGotFrame=false;
		}
	}
}

void H264RTP2Frame::PacketsToFrame(void)
{
	int nFrameLen=CalculateFrameSize()*2;
	if (nFrameLen>0)
	{
		if (m_pFrameBuf==NULL || m_nFrameBufLen<nFrameLen)
		{
			if (m_pFrameBuf!=NULL)
			{
				free(m_pFrameBuf);
				m_pFrameBuf=NULL;
			}
			m_nFrameBufLen=m_nHeaderMargin+nFrameLen;
			m_pFrameBuf=(unsigned char*)malloc(m_nFrameBufLen);
		}

		if (m_pFrameBuf==NULL)
		{
			FlushRTPPackets();
			return;
		}

		m_nKeyFrame=0;
		nFrameLen=GetFrame(m_pFrameBuf+m_nHeaderMargin,m_nFrameBufLen-m_nHeaderMargin);
		int nKeyFrame=m_nKeyFrame;
		m_nKeyFrame=0;

		//char szDebug[1024]="";
		//sprintf(szDebug,"nKeyFrame=%d nFrameLen=%d\n",nKeyFrame,nFrameLen);
		//OutputDebugString(szDebug);

		m_rCallback.OnH264RTP2FrameCallbackFramePacket(this,m_pFrameBuf+m_nHeaderMargin,nFrameLen,nKeyFrame);
	}
}

int H264RTP2Frame::get_codec_header_len(void)
{
	return 0;
}


int H264RTP2Frame::handle_rtp_packet(unsigned char*buf,int len,unsigned char*pFrameBuf,int nBufLen)
{
    unsigned char nal = buf[0];
    unsigned char type = (nal & 0x1f);
    int result= 0;
    unsigned char start_sequence[]= {0, 0, 0, 1};


	if (type==5)
	{
		m_nKeyFrame=1;
	}

    if (type >= 1 && type <= 23)
        type = 1;              // simplify the case. (these are all the nal types used internally by the h264 codec)
    switch (type) 
	{
    case 1:
        memcpy(pFrameBuf, start_sequence, sizeof(start_sequence));
        memcpy(pFrameBuf+sizeof(start_sequence), buf, len);
		result=sizeof(start_sequence)+len;
        break;
    case 24:                   // STAP-A (one packet, multiple nals)
        // consume the STAP-A NAL
        buf++;
        len--;
        // first we are going to figure out the total size....
        {
            int pass= 0;
            int total_length= 0;
            unsigned char *dst= pFrameBuf;

            for(pass= 0; pass<2; pass++) 
			{
                const unsigned char *src= buf;
                int src_len= len;

                do 
				{
#ifdef NETEC_ARM
					unsigned short nTemp;
					memcpy(&nTemp,((unsigned char*)src), 2);
					unsigned short nal_size = ntohs(nTemp);
#else
                    unsigned short nal_size = ntohs(*(unsigned short*)(src)); // this going to be a problem if unaligned (can it be?)
#endif

                    // consume the length of the aggregate...
                    src += 2;
                    src_len -= 2;

                    if (nal_size <= src_len) 
					{
                        if(pass==0) 
						{
                            // counting...
                            total_length+= sizeof(start_sequence)+nal_size;
                        } 
						else 
						{
                            memcpy(dst, start_sequence, sizeof(start_sequence));
                            dst+= sizeof(start_sequence);

							unsigned char nal_type = (*src & 0x1f);
							if (nal_type==5)
							{
								m_nKeyFrame=1;
							}

                            memcpy(dst, src, nal_size);
                            dst+= nal_size;
                        }
                    } 

                    // eat what we handled...
                    src += nal_size;
                    src_len -= nal_size;

                } while (src_len > 2);      // because there could be rtp padding..

                if(pass==0) 
				{
                    dst= pFrameBuf;
                } 
				else 
				{
                    result=total_length;
                }
            }
        }
        break;


    case 28:                   // FU-A (fragmented nal)
        buf++;
        len--;                  // skip the fu_indicator
        {
            // these are the same as above, we just redo them here for clarity...
            unsigned char fu_indicator = nal;
            unsigned char fu_header = *buf;   // read the fu_header.
            unsigned char start_bit = fu_header >> 7;
//            unsigned char end_bit = (fu_header & 0x40) >> 6;
            unsigned char nal_type = (fu_header & 0x1f);
            unsigned char reconstructed_nal;

            // reconstruct this packet's true nal; only the data follows..
            reconstructed_nal = fu_indicator & (0xe0);  // the original nal forbidden bit and NRI are stored in this packet's nal;
            reconstructed_nal |= nal_type;
			
			if (nal_type==5)
			{
				m_nKeyFrame=1;
			}

            // skip the fu_header...
            buf++;
            len--;

            if(start_bit) 
			{
                // copy in the start sequence, and the reconstructed nal....
                memcpy(pFrameBuf, start_sequence, sizeof(start_sequence));
                pFrameBuf[sizeof(start_sequence)]= reconstructed_nal;
                memcpy(pFrameBuf+sizeof(start_sequence)+sizeof(nal), buf, len);
				result=sizeof(start_sequence)+sizeof(nal)+len;
            } 
			else 
			{
                memcpy(pFrameBuf, buf, len);
				result=len;
            }
        }
        break;
    default:
        result= 0;
        break;
    }

    return result;
}
