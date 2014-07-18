#ifndef	SESSDESCRIBE_H_
#define	SESSDESCRIBE_H_
#include <vector>
#include <string>
#include "MetaObject.h"

//Session Description Protocol—http://en.wikipedia.org/wiki/Session_Description_Protocol
//以下注释中带有**的属性是可选的，而其余则是必须的
typedef struct
{
	std::string t;		//(time the session is active)
	std::string r;		//**(zero or more repeat times)
} TimeDescription;

typedef struct
{
	std::string m;		//(media name and transport address)
	std::string i;		//**(media title or information field)
	std::string c;		//**(connection information — optional if included at session level)
	std::vector<std::string> b;		//**(zero or more bandwidth information lines)
	std::string k;		//**(encryption key)
	std::vector<std::string> a;		//**(zero or more media attribute lines — overriding the Session attribute lines)
} MediaDescription;

typedef struct
{
	std::string v;		//(protocol version number, currently only 0)
	std::string o;		//(owner/creator and session identifier : username, id, version number, network address)
	std::string s;		//(session name : mandatory with at least one UTF-8-encoded character)
	std::string i;		//**(session title or short information)
	std::string u;		//**(URI of description)
	std::string e;		//**(zero or more email address with optional name of contacts)
	std::string p;		//**(zero or more phone number with optional name of contacts)
	std::string c;		//**(connection information-not required if included in all media)
	std::vector<std::string> b;		//**(zero or more bandwith information lines)
	//One or more Time descriptions("t=" and "r=" lines; see below)
	std::vector<TimeDescription> vtd;			//存放时间描述结构
	std::string z;		//**(time zone adjustments)
	std::string k;		//**(encryption key)
	std::vector<std::string> a;		//**(zero or more session attribute lines)
	//Zero or more Media descriptions(each one starting by an "m=" line; see below)
	std::vector<MediaDescription> vmd;		//存放媒体描述结构
} SessDescription;

struct _SDPCacheInfo
{
	MediaDescription* pASDescription;
	MediaDescription* pVSDescription;
	std::string strAudioID;			//存放媒体标识
	std::string strVideoID;
	//...

	_SDPCacheInfo()
		:pASDescription(NULL),
		 pVSDescription(NULL)
	{
	}
};
typedef struct _SDPCacheInfo		SDPCacheInfo;

class RZSessDescribe
{
public:
	RZSessDescribe();
	~RZSessDescribe() {}

public:
	inline std::string GetAudioID() const;
	inline std::string GetVideoID() const;
	int GetSampFrequence(STREAM_MEDIA_TYPE) const;
	void SetTypeValue(const std::string&);

private:
	void SetAVStreamIndex();
	void SetAVStreamID(const MediaDescription*);

private:
	SessDescription m_stSessDes;
	SDPCacheInfo m_stCacheInfo;
};

inline std::string RZSessDescribe::GetAudioID() const
{
	return m_stCacheInfo.strAudioID;
}

inline std::string RZSessDescribe::GetVideoID() const
{
	return m_stCacheInfo.strVideoID;
}
#endif		//SESSDESCRIBE_H_