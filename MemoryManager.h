#pragma once
#include "Process.h"
#include "BackingStore.h"
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <thread>
#include <mutex>

struct Proc {
    int pid;               // Process ID
    int memory;            // Process size
    std::string active;           // Indicates if the process is in memory: "running", "idle", "removed"
    long long time;        // Indicates how long this process has been residing in the memory
};

class MemoryManager {
private:
    std::vector<int> memory;            // Memory is represented as an array of frames
    std::vector<Proc> processes;
    std::vector<std::shared_ptr<Process>> processVector;
    int totalFragmentation = 0; //might remove since there is nothing in specs that asks for this
    int availableMemory = -1; // default value just for initialization

    int maxMemory = 16384;  // Total memory available (16KB)
    int frameSize = 16;     // Frame size (16 bytes)

    int numPagedIn = 0;

    std::string memType;

    BackingStore bs = BackingStore();

    bool flatAllocate(int pid, int processSize);
    bool pagingAllocate(int pid, int processSize);
    void deallocateOldest();
    bool isAllRunning();

    /*
    mutable std::mutex memoryMutex; // Protects memory and processes
    std::mutex backingStoreMutex;         // Protects interactions with BackingStore
    std::condition_variable_any cv;       // For signaling threads
    std::atomic<bool> stopThreads{ false };
    */

public:
    MemoryManager(int maxMemory, int frameSize, int availableMemory);
    bool allocate(std::shared_ptr<Process> process);
    bool isAllocated(int pid);
    bool isAllocatedIdle(int pid);
    void deallocateMemory(int pid);

    void setStatus(int pid, const std::string& status);

    int getActiveProcessesCount() const;
    int getAvailableMemory() const;
    void setAvailableMemory(int free);

    void printMemoryLayout(int cycle) const;
    std::string getCurrentTime() const;

    int getMaxMemory() const;
    int getUsedMemory() const;
    float getMemoryUtil() const;

    int getPagedIn() const;
    int getPagedOut() const;
    int getFlatPages() const;

    void printMemoryDetails(float cpuUtil);
};
