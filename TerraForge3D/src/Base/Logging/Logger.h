#include "LogHandler.h"


class Logger
{
public:
	Logger(std::string logDir);
	~Logger();

	LoggingOutputStreambuf* mLogHandler;
};