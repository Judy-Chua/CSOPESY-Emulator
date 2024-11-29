#pragma once
#include "MemoryManager.h"
#include "Process.h"
#include <queue>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <condition_variable>
#include <atomic>


class Scheduler {
public:
    Scheduler(int numCores, const std::string& type, int timeSlice, int freq, int min, int max, int delay, int memMax, int memFrame, int minMemProc, int maxMemProc);
    void addProcess(std::shared_ptr<Process> process);
    void startScheduling();
    void generateProcesses();
    void startTicksProcesses();
    void stopScheduler();
    void printActiveScreen();
    void reportUtil();
    void screenInfo(std::ostream& shortcut);

    void scheduleFCFS();
    void scheduleRR();

    int generateRandomNumber(int min, int max);
    int generateInstructions();
    int generateMemory();

    void printProcessSMI();
    void printVmstat();

    int getUsedCores();
    float getCpuUtilization();

    std::string makeSpaces(int input);
    std::string makeSpacesTicks(long long input);
    

    void startTicks();
    long long getActiveTicks();
    void incrementTicks(long long ticks);
    long long getIdleTicks();
    void incrementIdleTicks(long long ticks);

private:
    void schedule();
    void generateProcess();
    void worker(int coreId, std::shared_ptr<Process> process);
    void workerRR(int coreId, std::shared_ptr<Process> process);
    int countAvailCores();

    MemoryManager memoryManager;

    std::thread schedulerThread;
    std::thread generateProcessThread;
    std::thread ticksThread;
    std::thread printThread;
    std::vector<std::shared_ptr<Process>> processes;
    std::queue<std::shared_ptr<Process>> processQueue;
    std::vector<std::thread> workers;
    std::mutex queueMutex;
    std::mutex cpuMutex;
    std::mutex stateMutex;
    std::condition_variable cv;
    bool stop = false;
    bool stopPrinting = false;
    
    std::string type;
    std::vector<bool> coreAvailable;

    //std::atomic<long long> activeTicks{ 0 };
    //std::atomic<long long> idleTicks{ 0 };
    int numCores;
    int timeSlice = 0;
    int minIns, maxIns, batchFreq, delaysPerExec;
    int maxOverallMem, memPerFrame, minMemPerProc, maxMemPerProc;

    long long activeTicks, idleTicks;
};


