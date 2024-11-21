#include "MemoryManager.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <fstream>
#include <ctime>
// Constructor: Initialize memory with -1 (indicating free space)
MemoryManager::MemoryManager(int maxMemory, int frameSize, int availableMemory)
    : memory(maxMemory / frameSize, -1), maxMemory(maxMemory), frameSize(frameSize),
      availableMemory(availableMemory) {
    if (maxMemory == frameSize) {
        memType = "flat";
    }
    else {
        memType = "paging";
    }
}


bool MemoryManager::allocate(int pid) {
    if (memType == "flat") {
        return flatAllocate(pid);
    }
    
    return pagingAllocate(pid);
}


// Flat memory allocation
bool MemoryManager::flatAllocate(int pid) { //NEEDS FIXING
    
    // First-fit algorithm to find the first block of free memory
    for (int i = 0; i < memory.size(); ++i) {
        if (memory[i] == -1) {
            memory[i] = pid;
            processes.push_back({ pid, i, processMemory, true });
            availableMemory -= processMemory;
            return true;
        }
    }

    // If allocation fails, calculate external fragmentation
    totalFragmentation = maxMemory - processMemory;
    return false;
}

// Paging memory allocation
bool MemoryManager::pagingAllocate(int pid) {   //NEED FIXING PERO GOOD START ETO
    int requiredFrames = maxMemory / frameSize;
    int freeFrames = 0;
    int start = -1;

    for (int i = 0; i < memory.size(); ++i) {
        if (memory[i] == -1) {
            if (start == -1) start = i;
            ++freeFrames;
            if (freeFrames == requiredFrames) {
                for (int j = start; j < start + requiredFrames; ++j) {
                    memory[j] = pid;
                }
                processes.push_back({ pid, start * frameSize, (start + requiredFrames) * frameSize - 1, true });
                return true;
            }
        }
        else {
            freeFrames = 0;
            start = -1;
        }
    }

    // If allocation fails, calculate external fragmentation
    totalFragmentation = maxMemory - (processes.size() * processMemory);
    return false;
}


// Returns if process is already in the memory or not
bool MemoryManager::isAllocated(int pid) {
    bool allocated = false;
    for (int i = 0; i < memory.size(); ++i) {
        if (memory[i] == pid) {
            allocated = true;
            break;
        }
    }
    return allocated;
}

// Deallocate memory when the process finishes
void MemoryManager::deallocateMemory(int pid) {
    int freedMemory = 0;
    for (int i = 0; i < memory.size(); ++i) {
        if (memory[i] == pid) {
            memory[i] = -1;
            freedMemory += memPerProc;
        }
    }

    availableMemory += freedMemory;

    // Remove the process from the active list
    for (auto& p : processes) {
        if (p.pid == pid) {
            p.active = false;
            break;
        }
    }
}

// Print memory layout every quantum cycle
void MemoryManager::printMemoryLayout(int cycle) const {
    std::ofstream file("memory\\memory_stamp_" + std::to_string(cycle) + ".txt");
    file << "Timestamp: " << getCurrentTime() << "\n";
    file << "Number of processes in memory: " << getActiveProcessesCount() << "\n";
    file << "Total external fragmentation in KB: " << totalFragmentation / 1024 << "\n\n";

    file << "----end---- = " << maxMemory << "\n\n";
    for (const auto& p : processes) {
        if (p.active) {
            file << p.memoryEnd << "\n";
            file << "P" << p.pid << "\n";
            file << p.memoryStart << "\n\n";
        }
    }
    file << "----start---- = 0\n";
    file.close();
}

// Get the current timestamp
std::string MemoryManager::getCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    time_t currentTime = std::chrono::system_clock::to_time_t(now);
    struct tm buf;
    localtime_s(&buf, &currentTime);

    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "%m/%d/%Y %I:%M:%S %p", &buf);
    return timeStr;
}

int MemoryManager::getActiveProcessesCount() const {
    int count = 0;
    for (const auto& p : processes) {
        if (p.active) ++count;
    }
    return count;
}

int MemoryManager::getAvailableMemory() const { 
    return availableMemory;
}

void MemoryManager::setAvailableMemory(int free) { 
    this->availableMemory = free;
}

int MemoryManager::getMaxMemory() const {
    return maxMemory;
}

int MemoryManager::getUsedMemory() const {
    return getMaxMemory() - getAvailableMemory();
}

float MemoryManager::getMemoryUtil() const {
    int used = getUsedMemory();
    float util = float(used) / getMaxMemory() * 100;
    return util;
}