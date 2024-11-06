#pragma once
#include "Process.h"
#include <queue>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <condition_variable>
#include "MemoryManager.h"

class Scheduler {
public:
    Scheduler(int numCores, const std::string& type, int timeSlice, int freq, int min, int max, int delay, int memMax, int memFrame, int memProc);
    void addProcess(std::shared_ptr<Process> process);
    void startScheduling();
    void generateProcesses();
    void stopScheduler();
    void printActiveScreen();
    void reportUtil();
    void screenInfo(std::ostream& shortcut);

    void scheduleFCFS();
    void scheduleRR();

    int generateRandomNumber();

private:
    void schedule();
    void generateProcess();
    void worker(int coreId, std::shared_ptr<Process> process);
    void workerRR(int coreId, std::shared_ptr<Process> process);
    int countAvailCoresRR();
    int countAvailCores();
    
    // useless?
    //void printProcessQueue(const std::queue<std::shared_ptr<Process>>& processQueue);

    std::thread schedulerThread;
    std::thread generateProcessThread;
    std::thread printThread;
    std::vector<std::shared_ptr<Process>> processes;
    std::queue<std::shared_ptr<Process>> processQueue;
    std::vector<std::thread> workers;
    std::mutex queueMutex;
    std::condition_variable cv;
    bool stop = false;
    bool stopPrinting = false;
    
    std::string type;
    std::vector<bool> coreAvailable;

    int numCores;
    int timeSlice = 0;
    int minIns, maxIns, batchFreq, delaysPerExec, maxOverallMem, memPerFrame, memPerProc;
};


