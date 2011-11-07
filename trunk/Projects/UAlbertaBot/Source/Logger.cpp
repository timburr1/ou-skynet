#include "Common.h"
#include "Logger.h"

#define SELF_INDEX 0
#define ENEMY_INDEX 1

// gotta keep c++ static happy
Logger * Logger::instance = NULL;

// constructor
Logger::Logger() 
{
	logFile = "C:\\UalbertaBot_log.txt";
	logStream.open(logFile.c_str(), std::ofstream::app);
}

// get an instance of this
Logger * Logger::getInstance() 
{
	// if the instance doesn't exist, create it
	if (!Logger::instance) 
	{
		Logger::instance = new Logger();
	}

	return Logger::instance;
}

void Logger::setLogFile(const std::string & filename)
{
	logFile = filename;
}

void Logger::log(const std::string & msg)
{
	logStream << msg;
}