#ifndef	LOGGER_H_
#define	LOGGER_H_
#include <iostream>

#define DEBUG(X)		std::cout<<#X" = "<<X<<'\n'

class RZLogger
{
public:
	static void ERR(const char* fmt, ...);

private:
	RZLogger() {}
	~RZLogger() {}
};
typedef class RZLogger	Log;
#endif		//LOGGER_H_