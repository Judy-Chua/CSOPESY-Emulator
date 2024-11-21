#pragma once
#include "Process.h"
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <string>

class Worker {
public:

    Worker(int coreId, const std::string& type);

    void assignWork(std::shared_ptr<Process> process, int delay, int timeSlice);
    void executeFCFS();
    void executeRR();
    void startCountTicks();
    int getActiveTicks() const;
    int getIdleTicks() const;
    bool isAssigned() const;

private:
    int coreId;
    std::shared_ptr<Process> process;
    std::string type;
    int delay;
    int timeSlice;
    int activeTicks, idleTicks;
    bool assigned = false;
};