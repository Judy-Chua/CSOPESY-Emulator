#include "PrintCommand.h"
#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <sstream>
#include <vector>
#include <iomanip>

using namespace std;

PrintCommand::PrintCommand(const std::string& str) : name(str) {}

void PrintCommand::execute(int coreID) {
    std::ostringstream message;
    struct tm buf;
    char timeStr[100];

    auto now = std::chrono::system_clock::now();
    auto currentTime = std::chrono::system_clock::to_time_t(now);
    
    localtime_s(&buf, &currentTime);
    strftime(timeStr, sizeof(timeStr), "%m/%d/%Y %I:%M:%S %p", &buf);

    message << "(" << timeStr << ") Core: " << coreID
        << " - Executing command for process \"" << name << "\"";

    output.push_back(message.str());
}