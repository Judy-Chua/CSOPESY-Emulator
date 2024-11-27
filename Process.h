#pragma once
#include <string>
#include <mutex>
#include "PrintCommand.h"
using namespace std;

class Process {
public:
	enum ProcessState {
		READY, RUNNING, WAITING, FINISHED
	};

	Process(int pid, const std::string& name, int lines, const std::string& startTime, int memory);
	bool isFinished() const;
	int getPID() const;
	int getCommandCounter() const;
	int getLinesOfCode() const;
	int getCoreID() const;
	int getMemorySize() const;
	ProcessState getState() const;
	std::string getName() const;
	std::string getStartTime() const;
	std::string getEndTime() const;

	void setState(ProcessState newState);
	void setStartTime();
	void setEndTime();
	void setCoreID(int coreID);
	void executeCommand(int coreID);
	mutable std::mutex processMutex;

private:
	int pid;
	std::string name;
	int commandCounter;
	int coreID, memorySize;

	ProcessState currentState;
	int linesOfCode = 0;
	std::string startTime = "";
	std::string endTime = "";

	PrintCommand* command;
};
