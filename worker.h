#pragma once
#include "process.h"
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <atomic>

class Worker {
public:
    Worker(int coreId, int delay, int timeSlice);

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;

    Worker(Worker&& other) noexcept;
    Worker& operator=(Worker&& other) noexcept;

    ~Worker();

    void assignWork(const ProcessInfo& process);
    void executeRR();
    void startCountTicks();
    void stop();

    int getActiveTicks() const;
    int getIdleTicks() const;

    bool isAssigned() const;

private:
    int coreId;
    int delay;
    int timeSlice;

    int activeTicks;
    int idleTicks;

    bool assigned;
    bool running;

    ProcessInfo currentProcess;

    mutable std::mutex mtx;
    std::mutex stateMutex;
    std::thread workerThread;
    std::thread tickThread;
};