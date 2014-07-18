// "RZ" is short for Ricci.Z ;-)
①RTSPClient/0.0.1 (HP Streaming Media v2014.05.24)
		->一个单线程的初级版本
②RTSPClient/0.1.1 (HP Streaming Media v2014.07.01)
		->引入多线程和日志系统
//////////////////////////////////////////////////////////////////////////
RZNetConn		[Connection.h]
这是一个纯虚类，不能够被实例化，但它提供了一些具体的连接共有的公共接口，并且定义了
所有的子类都会用到的一些属性和方法，这样子类不必再单独定义，直接继承就可以了
通常对于一个网络连接来说，socket/ip/port都是必须的，这些是构成一个通信信道的
基础设施，注：对于一个具体的连接所使用的协议类型保存在端口对象中，这是因为有端口
不一定会有连接，但如果有连接那么一定会有端口
WSADATA wsa;
void InitWinSockDLL();
		->在windows下有一个初始化windows socket dll的函数
			在使用任何套接字前都需要先调用这个函数以声明需要使用套接字
void InitNetConn();
		->初始化网络连接，主要申请了用于接收数据的缓冲区，并清空读集合，并执行一些平台
			相关的初始化

int iSockFile;
		->iSockFile = -1 无效；否则有效
void CreateSocket();
		->创建套接字，根据端口中存放的协议类型来创建TCP或是UDP套接字
inline bool SocketValid() const;
		->测试当前套接字是否有效，在建立连接之前首先应测试套接字是否有效

RZNetIPAddr	nIPAddr;		//n means net
RZNetPort			nPort;
		->这个IP地址和端口构成的二元组标识一个通信对端的指定进程
			并不需要在这个类中定义一个表明本地端口的数据成员，这是因为有些连接
			的本地端口需要手动设置，而有些连接的本地端口则由系统提供
virtual void BuildConnection() = 0;
		->建立连接，虽然UDP协议基于无连接的方式和对端进行通信，但我们这里仍然
			提供一个建立连接的方法，只不过在具体实现时并不调用connect函数，这样就
			使得整个模型更加统一规范
virtual void SendDataToPeer(const std::string&) = 0;
virtual std::string RecvDataFromPeer() = 0;
		->向对端发送数据以及从对端接收数据，仍需要试具体的连接而定
inline std::string GetIPAddr() const;
		->获取当前连接的对端的IP地址
inline unsigned short GetPort() const;
		->获取当前连接的对端的端口号
inline void SetPeerIPAndPort(const RZNetIPAddr&, const RZNetPort&);
		->设置通信对端的IP地址和端口，具体的连接有具体的设置方式

char* rBuffer;			//r means receive
unsigned long ulBufSize;
		->连接对象发送的数据由用户指定，因此不需要发送缓冲区，连接本身只负责
			接收数据，但接收时需要接收的数据大小是未知的，所以就需要设置一个足够
			大的缓冲区，这个连接设置的默认缓冲区大小是4096字节
int GetSysRecvBufSize() const;
		->获取系统协议栈使用的缓冲区大小，根据这个值可以适当的调整应用内部的
			缓冲区大小
void SetSysRecvBufSize(int);
		->设置系统协议栈使用的缓冲区大小
void SetLocalBufSize(unsigned long);
		->设置当前连接本地用于接收数据的缓冲区的大小（暂时没有任何对象使用）

fd_set readSet;
int maxFd;	
		->具体的连接会用到等待通信对端响应的函数，有可能是同步的，也可能是异步的
			等待函数调用了select函数，因此需要一个fd_set类型的读集合
inline void ClearReadSet();
		->清除读集合
inline void AppSockInReadSet();
		->将这个连接的套接字加入读集合，同时更新maxFd
int WaitForPeerResponse(const WAIT_MODE&, long _milliSec = 0) const;
		->等待通信对端响应，可以使用同步和异步两种模式，若为异步，可继续设置等待时间
			_milliSec的单位为毫秒，返回值为就绪的文件描述符个数
inline bool HaveDataToRead() const;
		->以异步模式测试当前连接的套接字是否可读

//////////////////////////////////////////////////////////////////////////
RZTcpConn		[Connection.h]
这个类没有私有数据成员，其所有的数据都是从RZNetConn类继承得来的
virtual void BuildConnection();
		->如果IP地址和端口有效，那么创建套接字并连接到指定地址
virtual void SendDataToPeer(const std::string&);
		->将数据发送到通信对端，数据存放在string字符串中
virtual std::string RecvDataFromPeer();
		->接收数据并将其存放在内部缓冲区中，这个缓冲区继承自RZNetConn类

//////////////////////////////////////////////////////////////////////////
RZUdpConn		[Connection.h]
实际上对于UDP来讲是没有Client和Server的区别的，因为任意的2个通信对象之间是对等的
这也就是说，在本地会有一个指定的端口，无论哪个ip地址，只要它把数据发往这个端口
那么recvfrom这个调用都会负责把数据接收到指定缓冲区中
需要将这个本地端口作为其私有数据成员保存下来，因为RTSPClient在和对端通信时需要用到
这个数据
virtual void BuildConnection();
virtual void SendDataToPeer(const std::string&);
virtual std::string RecvDataFromPeer();
		->同上
inline RZNetPort GetLocalListenPort() const;
inline void SetLocalListenPort(const RZNetPort&);
		->获取及设置本地接收数据的端口

//////////////////////////////////////////////////////////////////////////
RZLogger		[Logger.h]
日志类应包含2个部分：①写控制台；②写文件（包含文本文档和结构化的数据库文件）
①写控制台：这部分主要用于调试，比如调用平台相关API时可能会返回出错，出错信息提示
错误源并展示出错码，主要用于快速定位bug并解决bug，一般调试时只要写控制台就可以了
②写文件，对于可以正常运行的程序，有可能（它是一个Server）需要一周7*24小时的运行，这个
时候需要程序做一些记录（比如用户的请求信息、响应情况等等），这些记录被用来观察这个程序
是否正常运行，一般这些记录都是简单地写文件，但是如果需要更精细的控制，则可能会设计相关
的表来结构化地存储这些记录，所以这个类有可能会写数据库文件（目前只包含写控制台部分）
static void ERR(const char* fmt, ...);
		->提示出错并打印出错的文件，函数以及行号，并打印一些User-Defined Information
			因为在windows VS上调试出错可能会使屏幕一闪而过，所以加入按任意键退出功能

//////////////////////////////////////////////////////////////////////////
RZNetPort			[NetBase.h]
这个类不存在默认构造函数，在定义这个类的对象的同时必须指定端口对应的协议类型
如果确实无法指定协议类型，那么就使用ENUM_UNK枚举值
inline bool PortValid() const;
		->测试当前端口值是否可用
inline NET_PRTTYPE GetPrtType() const;
		->获取端口对应的协议类型
inline void SetPrtType(NET_PRTTYPE);
		->设置端口对应的协议类型
inline unsigned short GetPortValue() const;
		->获取当前端口值
void SetPortValue(unsigned short);	
		->设置端口值
void RandSelectValidPort(NET_PORTTYPE);
		->随机选择指定范围内的一个有效端口，选择的端口总是偶数，这是因为
			RTP协议只用偶数端口！
bool PortOccupied(unsigned long);
		->测试参数指定的端口值是否已被其他进程所占用
NET_PORTTYPE GetPortType() const;
		->取得端口的类型（有①知名端口；②注册端口；③动态端口三种类型）
			目前这个方法没有什么用，但写在这里吧
inline void UpdateSystemOccupiedPort();
std::vector<unsigned long> GetLocalUsedPort(NET_PRTTYPE);
std::vector<unsigned long> GetLocalUsedTcpPort();
std::vector<unsigned long> GetLocalUsedUdpPort();
		->取得系统中正在使用的端口（平台相关）

//////////////////////////////////////////////////////////////////////////
RZNetIPAddr		[NetBase.h]
这个类提供一个默认构造函数和一个带字符串参数的构造函数，可以使用一个点分
十进制形式的字符串初始化这个类的对象
inline unsigned long GetULIPAddr() const;
		->取得IP地址的点分十进制串形式
inline std::string GetSTRIPAddr() const;
		->取得IP地址的无符号整形形式
void SetIPAddr(const std::string&);
		->设置IP地址
inline bool IPAddrValid() const;
		->当前IP地址是否可用，可用的意义仅仅表示这个IP地址是否合法
inline void InitIPAddr(const std::string&);
		->初始化IP地址，主要设置2个私有数据成员
bool IPAddrLegal(const std::string&) const;
		->检测IP地址是否合法，合法返回true，否则为false

//////////////////////////////////////////////////////////////////////////
RZReqLine			[NetBase.h]
RZExtraHdr
RZReqPacket
这3个类提供一个基于TCP协议的请求包，3个类都比较简单，但用的地方会比较多
封装一下用起来会很方便，并且提供的接口也比较少，不再做具体介绍

//////////////////////////////////////////////////////////////////////////
RZGlobalInit		[MetaObject.h]
这个类采用单例模式，定义了一个本类的静态成员对象，因此在进入main函数之前就调用了
这个类的构造函数。
RZGlobalInit()
		->在这里完成对整个系统的初始化，这个函数在程序的整个生命周期中只被调用一次

//////////////////////////////////////////////////////////////////////////
RZStream		[MetaObject.h]
这个类提供了一个所有的流都有可能用到的串分割方法
static std::vector<std::string> StreamSplit(const std::string& _p1, const char* _p2);
		->串分割方法，根据_p2对字符串_p1切割，返回_p1中所有以_p2分隔的子串，并丢弃分隔符

//////////////////////////////////////////////////////////////////////////
RZTypeConvert		[MetaObject.h]
这个类提供可能会用到的各种类型之间的转换
static int StrToInt(const std::string&, int _base);
		->根据_base将字符串转换为int，只支持2~16进制之间的字符串的转换
static std::string IntToString(const int&);
		->将整形转换为字符串

//////////////////////////////////////////////////////////////////////////
RZThread		[MetaObject.h]
这个类对平台相关的线程函数打包从而实现一个抽象的线程类，需要单独开一个线程完成任务的
对象的类可以直接继承这个类，并且只要实现线程的主函数就可以调用StartThread以开启一个
线程
void StartThread(int nCount = 1);
		->开始一个线程，参考表示开启线程的个数，默认为1
virtual void ThreadProc() = 0;
		->线程主函数，各个具体的类应该自己实现这个函数
static unsigned long InitThreadProc(void* lpdwThreadParam);
		->windows平台上创建线程需要用到的回调函数。在父线程中调用srand(time(NULL))使用当前时间做种子
			是没用的，因为rand()对于各线程是独立的 ，就是说每个线程在线程切换时都保存当前随机的状态。因此
			在开启一个子线程时最好先调用srand函数

//////////////////////////////////////////////////////////////////////////
RZAgent : public RZThread		[MetaObject.h]
RZAgent是具体Agent的一个抽象类
virtual void ConnetToPeer(const RZNetIPAddr&, const RZNetPort&);
		->所有Agent都会有连接到对端的操作，当某个Agent比较特殊时可以重写自己的这部分操作。
			由于TCP协议在建立连接时才会创建套接字，所以在这个操作中只对基于TCP协议的连接追
			加读集合
virtual void RecvPeerData(WAIT_MODE _eWaitMode = ENUM_SYN);
virtual void ParsePacket(std::string&) = 0;
		->接收对端数据并解析收到的数据，接收对端数据的操作大多大同小异，因此该抽象类提供一个
			已经实现的虚函数，当具体的Agent有自己接收数据的方式时同样可以重写这个方法
			ParsePacket指明处理收到的数据的方法
void InitLocalPort(const RZNetPort&);
		->初始化本地端口，只有使用UDP协议时才会调用这个方法，这是因为使用TCP连接时，本地
			端口是由系统自动选择的一个随机端口，初始化操作首先将本地端口与指定端口进行绑定，
			而在绑定操作时会创建套接字，因此在绑定完成之后，将套接字追加至读集合中

//////////////////////////////////////////////////////////////////////////
RZSessDescribe		[SessDescribe.h]
这个类主要用于存放关于某个原文件的信息，由于这个文件需要在通信两端进行传输，因而在传输
之前需要事先告知对端这个文件的信息，对于服务方来说这个类最主要的操作是存，而对请求方来
说则是取
在这个类中还定义了一个SDPCacheInfo结构，存储这个结构是为了减少某些检索操作较为频繁的
数据的计算量，当用到这些数据时，直接取计算好的缓存中的数据就可以了
inline std::string GetAudioID() const;
inline std::string GetVideoID() const;
		->获取sdp结构体中标识音视频流的ID
void SetTypeValue(const std::string&);
		->将字符串解析成sdp结构体，并设置索引以及媒体流的ID
			索引及媒体流ID存放在缓存信息结构体SDPCacheInfo中
void SetAVStreamIndex();
void SetMediaStreamID(const MediaDescription*);
		->设置音视频流索引以及ID

//////////////////////////////////////////////////////////////////////////
RZRTPAgent : public RZAgent		[RTPProtocol.h]
RZRTPAgent暂时主要用于接收对端的实时数据流，其中数据流是以UDP协议发送的
这个类中用到了比较复杂的自定义的结构体，这些结构体中字段的详细解释如下
struct _RTPHeader 
{
	RTPFirstByte		uFirByte;
	RTPSecByte		uSecByte;
			->以上2个联合中每个字段（包括位域）的解释存放在源文件的定义处
	unsigned short		SequenceNumber;
			->序列号：发送方在每发完一个包后就将该值增加1，接收方可以由这个域检测
				包的丢失以及恢复包的序列，序列号的初值是随机的
	char		TimeStamp[4];
			->时间戳：记录了该包中的数据的第一个字节的采样时刻，在一次会话开始时，
				时间戳初始化成一个初始值，时间戳的数值要随时间不断地增加
	unsigned long SSRC;
			->指示RTP包流的来源，在同一个RTP会话中不能有2个不同的SSRC值
	std::vector<std::string>		CSRC;
			->贡献源列表：有0~15项，每项32比特，用来标识对一个RTP混合器产生的新包
				有贡献的所有RTP包的源，这个列表中拥有的项是不确定的，项数由FirstByte中
				的CC标志位指明
};

struct _MediaMetaData;
		->在请求实时流时，有可能对应的文件非常大，从而如果在内存中完成恢复包序列的
			操作将会占用相当大的内存资源。解决办法是剥离UDP数据包中的RTP头，将其
			存放在内存中，而将UDP中的实时流部分存储在一个临时文件中，由于在临时文件
			中将会依次存放UDP数据包中的实时流，因此需要在内存中存放文件内的实时流的
			信息，这样这个结构就作为临时文件中整个乱序数据流的元数据而存在，在恢复包
			序列时只要先对这个结构进行排序就可以了，从而极大地节约了内存资源

struct _MediaPacketInfo;
{
			->在收到包后需要做如下处理：检查RTP中的P域——该位指示包的尾部是否有填充，
				如果没填充，那么整个包都是RTP包，如果有填充，包的最后两个字节表示填充的
				长度，应该要从这个包中去掉填充的字节
	int iFirstSeq;
	int iPackNum;
	bool bRecvAll;
			->当收到最后一个RTCP包时，包有可能并未全部接收完，因为是乱序发送的
	unsigned long ulSSRC;
			->这个同步源由RZRTSPAgent设置，在每次接受RTP包时都要将包的同步源标识
				符与这个值比对，若相同再继续处理，否则丢包
	FILE* pTmpFile;
	FILE* pOrdFile;
			->临时文件必须以可读可写的方式打开，因为其不仅涉及存放乱序的实时流，而且在
				数据流接收完毕之后，还必须从其中的指定位置读取数据块从而恢复流的正确顺序
				这两个文件的初始化必须在RZRTPAgent的构造函数中进行，这是因为必须根据RTP
				需要完成的任务来创建对应的文件，这样文件指针的释放也必须由RZRTPAgent来负责
	std::vector<MediaMetaData> vMetaData;
};
以上是RZRTPAgent类用到的结构体
std::queue<std::string> qBufferPool;
		->设置一个缓冲池，当接收到一个udp包时并不立即将包写进文件，而是等到缓冲池满时
			再一并写入文件，这样能够提高系统的I/O效率
void InitAgent();
		->初始化工作是设定本地的接收端口，选择的端口必须是连续2个都未被占用的端口中的
			第一个，并且第一个端口应该是偶数，而第二个端口则由RTCP协议使用。这2个端口在
			动态端口中进行选择
void InitTmpFile();
		->初始化临时文件，根据接收数据流的类型生成响应的文件并打开这些文件，在这个类的
			析构函数中关闭这些文件
void ThreadProc();
void ParsePacket(std::string&);
void WriteTmpFile();
static bool Compare(const MediaMetaData&, const MediaMetaData&);
void SortPacket();
		->ThreadProc是线程主函数，开启这个线程时首先打开RTCP线程，并不停地接收对端发送
			到本地的数据流。当接收到一个数据包之后，使用ParsePacket解析这个包，并将数据流的
			有效载荷存储在本地缓冲池，当缓冲池满或者已接收完全部数据流之后，再调用
			WriteTmpFile写入临时文件，注意此时写入的数据流是乱序的。接受完所有的数据流之后
			退出主循环，对乱序的数据流排序并生成有序文件
inline RZNetPort GetLocalPort() const;
		->获取本地的接收端口
inline void SetMediaType(RTP_MEDIA_TYPE);
		->设置媒体类型
inline void SetFirstSeq(int);
		->设置第一个RTP包的序列号
inline void SetSSRC(unsigned long);
		->设置RTP包的SSRC（同步源标识符）
inline bool PackRecvComplete() const;
		->测试RTP包是否已全部接受完，若是则返回true，否则返回false

//////////////////////////////////////////////////////////////////////////
RZRTCPAgent : public RZAgent		[RTPProtocol.h]
RTCP是RTP的控制部分，其主要为RTP实时流的服务质量提供保证，所以在RTCP包中不包含
实体，只有头部，并且一个RTCP包通常会包含多个头部！
RTCP包共有5种类型的头部，分别是：
①SR（200）——发送端报告；②RR（201）——接收端报告；③SDES（202）——源点描述
④BYE（203）——结束传输；⑤APP（204）——特定应用
通常由一个头部的第二个字节（PT域）指明头部的类型
在这个实现中只考虑从服务器端拉流，并不考虑实时流的质量控制，所以只定义SR头部结构，
因为其中包含和RTP数据包有关的信息【SenderInfo中的PacketCount/PayloadOctets字段】，
在读到RTCP包中的其他头部时简单忽略，当检测到BYE头部时就可以知道已发送的RTP包的总数
RTCP_Packet rPacket;
		->表示从对端收到的响应包
inline void SetLocalSSRC(unsigned long);
		->设置本地SSRC，通常在开始接收RTCP包之前设置，在收到RTCP包之后要检查SSRC是否
			相等
inline bool GetLastPacket() const;
inline unsigned long GetTotalRTPPack() const;
		->指明是否已取得最后一个RTCP包，并获取RTP包的总数。在接收到最后一个RTCP包时，
			这个包中含有对端发送的RTP包的总数
void ThreadProc();
void ParsePacket(std::string&);
		->ThreadProc是线程主函数，开启这个线程主要用于接收RTCP包，但不对这些包响应，接收
			的目的是取得最后一个RTCP包并得到对端发送的RTP包的总数
			ParsePacket主要是解析RTCP包

//////////////////////////////////////////////////////////////////////////
RZRTSPAgent : public RZAgent		[RTSPAgent.h]
RTSP通过与RTP/RTCP协议配合进行流的实时传输，RTSP与服务器建立TCP连接，而数据流
则通过基于UDP的RTP协议进行传输
RTSP_TASK			eTask;
		->定义任务，即这个Agent与对端端通信是为了完成什么
std::string strReqFile;
		->RZRTSPAgent请求得到的实时数据流是与某个具体的文件对应的
RZRTPAgent* pRTPAgent;
		->用于接收实时数据流，因为实时流可能是单独的视频流，单独的音频流，也可能是混合的
			音视频流，所以可能只需要1个对象，也可能会需要2个对象
struct _RTSPHdrCache
{
	int iCseq;
			->表示当前请求的序列号，每发送一次请求序列号都自增1
	std::string strSession;
			->Session头首次出现在Server对SETUP请求的响应中，这个头部由Server首先发送
				一旦发现响应包中出现Session头，那么后续的每个请求包中都需要包含该请求头
};
ResponsePacket	stResPacket;
RTSPHdrCache		stHdrCache;
RZSessDescribe	sdpPacket;
		->分别表示从对端接收的响应包，以及响应包中的缓存信息，会话描述实体
void ConnectToServer(const char*);
		->连接到服务器，之所以不需要指定端口，是因为这里使用了RTSP通信的默认端口
inline void SetRequestFile(const std::string&);
		->设置请求文件
void StartTask(RTSP_TASK);
		->执行任务
std::string			GetMethodStr(RTSP_METHOD) const;
		->根据枚举值获取对应方法的字符串
RZReqLine		GetReqLineByMethod(RTSP_METHOD, int);
		->根据方法获取对应的请求行。一个请求行由<method> <uri> <version>组成，其中uri（
			即通用资源定位符）通常只会用到指定的地址和指定的文件。但由于RTSP请求的是实时的
			视音频流，而这2个流又放置在一个统一的文件中，因此RTSP的某些方法对应的uri可能
			还会需要指定文件内部的某一块数据（可能是视频流也可能是音频流），所以uri可能是
			rtsp://ipaddr:port/requestfile，或者rtsp://ipaddr:port/requestfile/specifydatablock
			而使用到后一种uri的方法只有少数几个，并且对于这个任务来说只有SETUP方法会用到
std::string			GetHeaderFieldStr(RTSP_HEADER_FIELD) const;
		->根据枚举值获取对应报头域的字符串
std::vector<RZExtraHdr> GetGeneralExtraHdr() const;
void GetOPTIONSExtraHdr(std::vector<RZExtraHdr>&) const;
void GetDESCRIBEExtraHdr(std::vector<RZExtraHdr>&) const;
void GetSETUPExtraHdr(std::vector<RZExtraHdr>&, int) const;
void GetPLAYExtraHdr(std::vector<RZExtraHdr>&) const;
void GetTEARDOWNExtraHdr(std::vector<RZExtraHdr>&) const;
		->获取通用的附加报头及指定方法的附加报头。由于每种方法所需要的请求报头并没有一个
			标准的规定，因此这里参考VLC客户端实现中
void OnDispatchAcceptAVStream();
		->实现接收实时流的具体方法
std::vector<RTSP_METHOD> GetTaskMethodList() const;
		->获得与具体任务对应的方法列表。接收请求文件中的视/音频流参考VLC中的实现，
			具体到方法的发送顺序如下：OPTIONS->DESCRIBE->*(SETUP[Audio])->
			*(SETUP[Video])->PLAY->TEARDOWN
void SendRequest(RTSP_METHOD, int);
		->发送请求至服务器端，在每次发送之前都要首先将序列号自增1
void ParsePacket(std::string&);
		->解析响应包并填充stResPacket成员，但是在每次填充之前并不将这个成员清空，这意味着
			在某个时间戳上，该成员内的每个字段并非都属于当前请求的响应包。在解析完响应行和报头
			之后可能还有实体，但对实体部分不做任何解析，因为不同任务可能会使用不同的文本描述
			实体部分的解析由具体的任务来执行
std::map<std::string, int>					GetRTPFirstSeq() const;
		->获取RTP数据流中第一个包的序列号，由于有可能发送多个不同类型的数据流，所以在响应
			包的RTP-Info报头中会有多个以“，”分隔的值。这个函数的返回值是一个map容器，其中
			key表示数据流的标识符—即uri中由“/”分隔的最后一个字符串，value表示包的序列号
std::map<std::string, std::string>		GetResTransFields() const;
		->获取响应包的transport报头中有关对端端口的信息，该函数也返回一个map容器，其中key
			的值为"server_port"，value表示server_port的值，类似于"57402-57403"
void OnDispatchAcceptAVStream();
		->在运行RTSP协议的两端之间的通信应该是同步的，换言之，当客户端发送一个请求之后，
			客户端应该在此等待，只有得到了Server端的响应才能根据响应的内容决定是否继续发送
			下一个请求。如果他们之间的通信是异步的，可以考虑这样一种状况：服务器发送一个
			DESCRIBE方法之后，并不等待服务器的回复就立即发送SETUP方法，这显然是错误的，
			因为SETUP方法中具体到和某一块数据（音频/视频流）相关的标识（这个标识构成URI）
			还没有被客户端接收到
			在接收到PLAY方法的响应包之后，RTSPAgent被阻塞，直到RTPAgent接收完全部的实时
			数据流。响应包中只有Range和RTP-Info这2个报头需要特别处理，对Range的处理方式是
			直接显示该报头的内容，而RTP-Info报头中的某些值需要在接收实时流时由RTPAgent使用