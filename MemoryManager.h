#pragma once
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <fstream>

struct Proc {
    int pid;               // Process ID
    int memory;            // Process size
    bool active;           // Indicates if the process is in memory
    long long time;        // Indicates how long this process has been residing in the memory
};

class MemoryManager {
private:
    std::vector<int> memory;            // Memory is represented as an array of frames
    std::vector<Proc> processes;
    int totalFragmentation = 0; //might remove since there is nothing in specs that asks for this
    int availableMemory = -1; // default value just for initialization

    int maxMemory = 16384;  // Total memory available (16KB)
    int frameSize = 16;     // Frame size (16 bytes)

    std::string memType;

    bool flatAllocate(int pid, int processSize);
    bool pagingAllocate(int pid, int processSize);
    void deallocateOldest();

public:
    MemoryManager(int maxMemory, int frameSize, int availableMemory);
    bool allocateMemory(int pid, int processSize);
    bool isAllocated(int pid);
    void deallocateMemory(int pid);
    int getActiveProcessesCount() const;
    int getAvailableMemory() const;
    void setAvailableMemory(int free);

    int getMaxMemory() const;
    int getUsedMemory() const;
    float getMemoryUtil() const;
};
