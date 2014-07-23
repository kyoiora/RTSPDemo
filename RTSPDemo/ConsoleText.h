#ifndef	CONSOLETEXT_H_
#define	CONSOLETEXT_H_

#ifdef WIN32
#ifndef	WIN32_LEAN_AND_MEAN
#define	WIN32_LEAN_AND_MEAN
#endif
#define	_CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#elif
#endif

#include "MetaObject.h"

class RZConsoleOutput 
{
public:
	RZConsoleOutput();
	~RZConsoleOutput();

public:
	void GetCursorPosition();
	void OutputCharacter(const char*, ...);

private:
#ifdef WIN32
	HANDLE m_hConsoleOutput;
	COORD m_dwWriteCoord;		//ÆðÊ¼Î»ÖÃ
#endif
};

#endif		//CONSOLETEXT_H_