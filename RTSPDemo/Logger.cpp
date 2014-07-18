#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "Logger.h"

void RZLogger::ERR(const char* fmt, ...)		//函数调用出错，打印信息就退出
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
#ifdef	WIN32
	system("pause>nul");		//按任意键退出
#endif
	exit(EXIT_FAILURE);
}