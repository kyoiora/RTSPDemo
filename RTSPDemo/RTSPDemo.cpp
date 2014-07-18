// RTSPDemo.cpp : 定义控制台应用程序的入口点。
//

#include "RTSPAgent.h"

const std::string CServerAddr = "192.168.4.134";
const std::string CRequestFile = "live1.sdp";

int main(int argc, char* argv[])
{
	RTSPAgent rAgent;
	rAgent.ConnectToServer(CServerAddr);
	rAgent.SetRequestFile(CRequestFile);
	rAgent.StartTask(ENUM_ACCEPTVS);

	while(1);
	return 0;
}

