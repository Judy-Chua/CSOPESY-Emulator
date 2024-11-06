#pragma once
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <fstream>

struct Proc {
    int pid;
    int memoryStart;
    int memoryEnd;
    bool active;
};

class MemoryManager {
private:
    std::vector<int> memory;            // Memory is represented as an array of frames
    std::vector<Proc> processes;
    int totalFragmentation = 0;
    int availableMemory = -1; // default value just for initialization

    int maxMemory = 16384;  // Total memory available (16KB)
    int memPerProc = 4096; // Memory required per process (4KB)
    int frameSize = 16;     // Frame size (16 bytes)

public:
    MemoryManager(int maxMemory, int memPerPoc, int frameSize, int availableMemory);
    bool allocateMemory(int pid);
    void deallocateMemory(int pid);
    void printMemoryLayout(int cycle) const;
    int getActiveProcessesCount() const;
    std::string getCurrentTime() const;
    int getAvailableMemory() const;
    void setAvailableMemory(int free);
};
