#include "Process.h" 
#include <fstream>
#include <ctime>
#include <iostream>
#include <chrono>
#include <mutex>
#include "PrintCommand.h"

using namespace std;

Process::Process(int pid, const std::string& name, int lines, const std::string& startTime, int memory) {
    this->pid = pid;
    this->name = name;
    this->currentState = READY;
    this->commandCounter = 0;
    this->linesOfCode = lines;
    this->coreID = -1;
    this->startTime = startTime;
    this->memorySize = memory;
    command = new PrintCommand(name);
}

bool Process::isFinished() const {
    return currentState == FINISHED;
}

int Process::getPID() const {
    return pid;
}

int Process::getCommandCounter() const {
    return commandCounter;
}

int Process::getLinesOfCode() const {
    return linesOfCode;
}

void Process::setState(ProcessState newState) {
    currentState = newState;
}

Process::ProcessState Process::getState() const {
    return currentState;
}

std::string Process::getName() const {
    return name;
}

void Process::setCoreID(int coreID) {
    this->coreID = coreID;
}


void Process::executeCommand(int coreID) {
    if (isFinished())
        return;

    if (currentState == Process::WAITING)
        return;

    this->coreID = coreID;
    std::lock_guard<std::mutex> lock(processMutex);

    if (currentState == RUNNING && commandCounter < linesOfCode) {
        command->execute(coreID);
        commandCounter++;
    }

    if (commandCounter >= linesOfCode) {
        currentState = FINISHED;
        setEndTime();
    }
}

void Process::setStartTime() {
    auto now = chrono::system_clock::now();
    time_t currentTime = chrono::system_clock::to_time_t(now);
    struct tm buf;
    localtime_s(&buf, &currentTime);

    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "(%m/%d/%Y %I:%M:%S %p)", &buf);

    startTime = timeStr;
}

void Process::setEndTime() {
    auto now = chrono::system_clock::now();
    time_t currentTime = chrono::system_clock::to_time_t(now);
    struct tm buf;
    localtime_s(&buf, &currentTime);

    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "(%m/%d/%Y %I:%M:%S %p)", &buf);

    endTime = timeStr;
}

std::string Process::getStartTime() const {
    return startTime;
}

std::string Process::getEndTime() const {
    return endTime;
}

int Process::getCoreID() const {
    return coreID;
}

int Process::getMemorySize() const {
    return memorySize;
}