#include "ConsoleText.h"
#include "Logger.h"

RZConsoleOutput::RZConsoleOutput()
{
#ifdef WIN32
	m_hConsoleOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (m_hConsoleOutput == INVALID_HANDLE_VALUE)
		Log::ERR("Platform SDK \'GetStdHandle\' called failed.\tError Code: %d.\n", ::GetLastError());
	::memset(&m_dwWriteCoord, 0, sizeof(COORD));
#endif
}

RZConsoleOutput::~RZConsoleOutput()
{
#ifdef WIN32
	if (::CloseHandle(m_hConsoleOutput) == 0)
		Log::ERR("Platform SDK \'CloseHandle\' called failed.\tError Code: %d.\n", ::GetLastError());
#endif
}

void RZConsoleOutput::GetCursorPosition()
{
#ifdef WIN32
	CONSOLE_SCREEN_BUFFER_INFO bInfo;
	if (::GetConsoleScreenBufferInfo(m_hConsoleOutput, &bInfo) == 0)
		Log::ERR("Platform SDK \'GetConsoleScreenBufferInfo\' called failed.\tError Code: %d.\n", ::GetLastError());
	m_dwWriteCoord = bInfo.dwCursorPosition;
#endif
}

void RZConsoleOutput::OutputCharacter(const char* fmt, ...)
{
	char buffer[256];
	::memset(buffer, 0, 256);
	va_list args;
	va_start(args, fmt);
	vsprintf(buffer, fmt, args);
	va_end(args);
#ifdef WIN32
	unsigned long nCharsWritten;
	if (::WriteConsoleOutputCharacter(m_hConsoleOutput, buffer, ::strlen(buffer), m_dwWriteCoord, 
																	&nCharsWritten) == 0)
	{
		Log::ERR("Platform SDK \'WriteConsoleOutputCharacter\' called failed.\tError Code: %d.\n", ::GetLastError());
	}
#endif
}