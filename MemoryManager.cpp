#include "MemoryManager.h"
#include "BackingStore.h"
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

// allocate based on type
bool MemoryManager::allocate(std::shared_ptr<Process> process) {
    
    for (auto& p : processes) {
        if (p.active == "running")
            p.time++;
    }
    bs.addProcess(process, process->getPID());

    if (memType == "flat") {
        return flatAllocate(process->getPID(), process->getMemorySize());
    }
    return pagingAllocate(process->getPID(), process->getMemorySize());
}

// First-fit memory allocation
bool MemoryManager::flatAllocate(int pid, int processSize) {
    // First-fit algorithm to find the first block of free memory

    if (isAllocatedIdle(pid)) {
        setStatus(pid, "running");
        return true;
    }
    
    if (availableMemory >= processSize) {
        processes.push_back({ pid, processSize, "running", 1}); // Record process information
        availableMemory -= processSize;
        //std::cout << "recorded " << pid << " so available memory now is " << availableMemory << std::endl;
        return true;
    } 
    
    if (!isAllRunning()) { // If allocation fails, reallocate by removing oldest process
        deallocateOldest();
        flatAllocate(pid, processSize);
    }
    return false;
}

// Paging memory allocation
bool MemoryManager::pagingAllocate(int pid, int processSize) {
    
    int requiredFrames = (processSize + frameSize - 1) / frameSize; // Calculate the number of frames needed (ceil division)
    int freeFrames = 0;
    int frameCtr = 0;
    std::vector<int> listOfFrames;

    for (int i = 0; i < memory.size(); ++i) {
        if (memory[i] == -1) { // Frame is free
            listOfFrames.push_back(i);
            ++freeFrames;
            if (freeFrames == requiredFrames) { // Enough frames found
                for (int j = 0; j < memory.size(); ++j) {
                    if (listOfFrames[frameCtr] == j) {
                        memory[j] = pid; // Allocate frames to the process
                        frameCtr++;
                    }
                }

                processes.push_back({ pid, processSize, "running", 1}); // Record process information
                    // Process ID, Process Size, Allocation status, time
                return true; // Allocation successful
            }
        }
    }

    // If allocation fails, reallocate by removing oldest process
    if (!isAllRunning() && freeFrames != requiredFrames) {
        deallocateOldest();
        pagingAllocate(pid, processSize);
    }
    return false;
}

// Returns if process is already in the memory or not
bool MemoryManager::isAllocated(int pid) {
    bool allocated = false;
    for (auto& p : processes) {
        if (p.pid == pid && (p.active == "running" || p.active == "idle")) {
            allocated = true;
            break;
        }
    }
    return allocated;
}

bool MemoryManager::isAllocatedIdle(int pid) {
    bool allocated = false;
    for (auto& p : processes) {
        if (p.pid == pid && p.active == "idle") {
            allocated = true;
            break;
        }
    }
    return allocated;
}

bool MemoryManager::isAllRunning() {
    bool running = true;
    for (auto& p : processes) {
        if (p.active != "running") {
            running = false;
            break;
        }
    }
    return running;
}

void MemoryManager::setStatus(int pid, const std::string& status) {
    for (auto& p : processes) {
        if (p.pid == pid) {
            p.active = status;
            break;
        }
    }
}

void MemoryManager::deallocateOldest() {
    
    int highest = 0;
    int oldestProcess = 0;

    std::cout << "deallocating lowest since no space " << std::endl;
    for (auto& p : processes) {
        if (p.time > highest && p.active == "idle") {
            highest = p.time;
            oldestProcess = p.pid;
        }
    }
    std::cout << "oldestprocess is " << oldestProcess << std::endl;
    bs.storeProcess(oldestProcess);     //backing store
    deallocateMemory(oldestProcess);    //deallocate now
}

// Deallocate memory when the process finishes
void MemoryManager::deallocateMemory(int pid) {
    
    int freedMemory = 0;

    if (memType == "flat") {
        for (auto& p : processes) {
            if (p.pid == pid) {
                freedMemory = p.memory;
                break;
            }
        }
    }
    else {
        for (int i = 0; i < memory.size(); ++i) {
            if (memory[i] == pid) {
                memory[i] = -1;
                freedMemory += frameSize;
            }
        }
    }
   
    availableMemory += freedMemory;
    std::cout << "deallocated process " << pid << " so available memory now is " << availableMemory << std::endl;

    // Remove the process from the active list
    for (auto& p : processes) {
        if (p.pid == pid) {
            p.active = "removed";
            break;
        }
    }
}

void MemoryManager::printMemoryLayout(int cycle) const {
    std::ofstream file("memory\\memory_stamp_" + std::to_string(cycle) + ".txt");
    file << "Timestamp: " << getCurrentTime() << "\n";
    file << "Number of processes in memory: " << getActiveProcessesCount() << "\n";
    file << "Total external fragmentation in KB: " << totalFragmentation / 1024 << "\n\n";
    file << "----end---- = " << maxMemory << "\n\n";
    for (const auto& p : processes) {
        if (p.active == "running" || p.active == "idle") {
            file << "status " << p.active << "\n";
            file << "P" << p.pid << "\n";
            file << p.memory << "\n\n";
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
        if (p.active == "running" || p.active == "idle") ++count;
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
