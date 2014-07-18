#include "SessDescribe.h"
#include "Logger.h"

RZSessDescribe::RZSessDescribe()
	:m_stSessDes(),
	 m_stCacheInfo()
{
}

void RZSessDescribe::SetTypeValue(const std::string& _strSDP)
{
	TimeDescription stTimeDes;
	MediaDescription stMediaDes;
	//sdp文本行的格式为<type>=<value>["\r\n"]，其中<type>是一个字母，<value>是一个文本串
	std::vector<std::string> vTextLine;
	std::vector<std::string> vTextLineList = RZStream::StreamSplit(_strSDP, "\r\n");

	for (std::vector<std::string>::const_iterator iter = vTextLineList.begin();
			iter != vTextLineList.end(); iter++)
	{
		vTextLine.push_back(iter->substr(0, 1));		//类型占用当前字符串的第一个字节
		vTextLine.push_back(iter->substr(2));			//从第二个字节开始表示该类型的值
		//一旦遇到无法识别的类型，那么简单忽略
		switch(vTextLine[0][0])		//带有//**注释的类型表示可选的
		{
		case 'v':		m_stSessDes.v	= vTextLine[1];	break;
		case 'o':		m_stSessDes.o	= vTextLine[1];	break;
		case 's':		m_stSessDes.s	= vTextLine[1];	break;
		case 'i':		m_stSessDes.i	= vTextLine[1];	break;		//**
		case 'u':		m_stSessDes.u	= vTextLine[1];	break;		//**
		case 'e':		m_stSessDes.e	= vTextLine[1];	break;		//**
		case 'p':		m_stSessDes.p	= vTextLine[1];	break;		//**
		case 'c':		m_stSessDes.c	= vTextLine[1];	break;		//**
		case 'b':		m_stSessDes.b.push_back(vTextLine[1]);		break;		//**

		case 't':
			{
				//在TimeDescription结构中，且字段t必定存在
				stTimeDes = TimeDescription();
				stTimeDes.t	= vTextLine[1];
				//若下一行已到达容器尾端，则直接退出这个语句
				if ((++iter) == vTextLineList.end()) 
				{
					iter--;	
					break; 
				}
				vTextLine.clear();
				vTextLine.push_back(iter->substr(0, 1));
				vTextLine.push_back(iter->substr(2));
				if (vTextLine[0][0] == 'r') 
					stTimeDes.r	= vTextLine[1];		//**
				else 
					iter--;
				m_stSessDes.vtd.push_back(stTimeDes);
				break;
			}

		case 'z':		m_stSessDes.z	= vTextLine[1];	break;		//**
		case 'k':		m_stSessDes.k	= vTextLine[1];	break;		//**
		case 'a':		m_stSessDes.a.push_back(vTextLine[1]);	break;		//**

		case 'm':
			{
				//在MediaDescription结构中，且字段m必定存在
				stMediaDes = MediaDescription();		//先清空
				stMediaDes.m	= vTextLine[1];
				for (iter++; iter != vTextLineList.end(); iter++)
				{
					vTextLine.clear();
					vTextLine.push_back(iter->substr(0, 1));
					vTextLine.push_back(iter->substr(2));
					if (vTextLine[0] == "m")
						break;			//此时已经进入下一个MediaDescription结构体中，直接退出
					switch(vTextLine[0][0]) 
					{
					case 'i':		stMediaDes.i		= vTextLine[1];	break;		//**
					case 'c':		stMediaDes.c	= vTextLine[1];	break;		//**
					case 'b':		stMediaDes.b.push_back(vTextLine[1]);	break;		//**
					case 'k':		stMediaDes.k = vTextLine[1];	break;		//**
					case 'a':		stMediaDes.a.push_back(vTextLine[1]);	break;		//**
					}
				}
				m_stSessDes.vmd.push_back(stMediaDes);
				iter--;
				break;
			}
		}
		vTextLine.clear();
	}

	//完成整个sdp串的解析，接下来建立索引及缓存
	SetAVStreamIndex();
	SetAVStreamID(m_stCacheInfo.pASDescription);
	SetAVStreamID(m_stCacheInfo.pVSDescription);
}

void RZSessDescribe::SetAVStreamIndex()
{
	std::vector<std::string> vTextLine;
	for (std::vector<MediaDescription>::iterator iter = m_stSessDes.vmd.begin();
			iter != m_stSessDes.vmd.end(); iter++)
	{
		//类型m的值以空格符分隔
		vTextLine = RZStream::StreamSplit(iter->m, " ");
		if (vTextLine[0] == "audio")				//音频流
			m_stCacheInfo.pASDescription = &(*iter);
		else if (vTextLine[0] == "video")		//视频流
			m_stCacheInfo.pVSDescription = &(*iter);
		else			//未知数据流，忽略
			;
	}
}

void RZSessDescribe::SetAVStreamID(const MediaDescription* _pMediaDes)
{
	if (_pMediaDes == NULL)
		return;

	std::string strMediaID = "";
	std::vector<std::string> vTextLine;
	for (std::vector<std::string>::const_iterator iter = (_pMediaDes->a).begin(); 
			iter != (_pMediaDes->a).end(); iter++)
	{
		vTextLine = RZStream::StreamSplit(*iter, ":");		//属性名和值之间以:分隔
		if (vTextLine[0] == "control")
		{
			strMediaID = vTextLine[1];
			break;
		}
	}
	if (_pMediaDes == m_stCacheInfo.pASDescription)
		m_stCacheInfo.strAudioID = strMediaID;
	else if (_pMediaDes = m_stCacheInfo.pVSDescription)
		m_stCacheInfo.strVideoID = strMediaID;
	else
		;
}

int RZSessDescribe::GetSampFrequence(STREAM_MEDIA_TYPE _eMedia) const
{
	if (_eMedia == ENUM_MEDIA_UNK)
		Log::ERR("Unknown media type, please set media type correctly.\n");

	int iSampFreq = 0;
	MediaDescription* pStreamDes = NULL;
	if (_eMedia == ENUM_AUDIO)
		pStreamDes = m_stCacheInfo.pASDescription;
	else			//_eMedia == ENUM_VIDEO
		pStreamDes = m_stCacheInfo.pVSDescription;
	std::vector<std::string> vTextLine;
	for (std::vector<std::string>::const_iterator iter = (pStreamDes->a).begin();
			iter != (pStreamDes->a).end(); iter++)
	{
		vTextLine = RZStream::StreamSplit(*iter, ":");
		if (vTextLine[0] == "rtpmap")
		{
			vTextLine = RZStream::StreamSplit(vTextLine[1], "/");
			iSampFreq = RZTypeConvert::StrToInt(vTextLine[1], 10);
		}
	}
	return iSampFreq;
}