#pragma once
#include <memory>
#include "Process.h"
#include "ConsoleManager.h"
#include <thread>
#include <chrono>
#include <mutex>

class Worker {
public:
    Worker(int coreId, std::shared_ptr<Process> process, const std::string& type, int delay, int timeSlice = 0);

    void execute();

private:
    int coreId;
    std::shared_ptr<Process> process;
    std::string type;
    int delay;
    int timeSlice;
};