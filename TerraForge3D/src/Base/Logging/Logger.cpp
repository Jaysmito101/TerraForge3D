#include "Logger.h"

#include <ctime>
#include <iostream>

Logger::Logger(std::string logsDir)
{
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time (&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer,sizeof(buffer),"%A %d-%m-%Y %I-%M-%S %p",timeinfo);
	std::string str(buffer);
	str += ".txt";	

	mLogHandler = new LoggingOutputStreambuf(std::cout, logsDir + "\\" + str);
}

Logger::~Logger()
{
	delete mLogHandler;
}