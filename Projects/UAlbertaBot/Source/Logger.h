#pragma once

#include "Common.h"
#include "BWTA.h"
#include <fstream>

class Logger {

	std::ofstream logStream;
	std::string logFile;

	Logger();
	static Logger *				instance;

public:

	static Logger *	getInstance();
	void log(const std::string & msg);
	void setLogFile(const std::string & filename);
};
