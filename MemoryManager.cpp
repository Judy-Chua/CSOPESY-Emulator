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
    bool isExistingAndRunning = false;
    for (auto& p : processes) {
        if (p.active == "running") {
            p.time++;
			if (p.pid == process->getPID()) {
				isExistingAndRunning = true;
			}
        }
    }

    if (isExistingAndRunning) {
        return false;
    }
     
    bs.addProcess(process, process->getPID());

    if (memType == "flat") {
        return flatAllocate(process->getPID(), process->getMemorySize());
    }
    return pagingAllocate(process->getPID(), process->getMemorySize());
}

// First-fit memory allocation
bool MemoryManager::flatAllocate(int pid, int processSize) {
    if (isAllocatedIdle(pid)) {
        setStatus(pid, "running");
        return true;
    }
    
    if (availableMemory >= processSize) {
        bool existing = false;
        for (auto& p : processes) {
            if (p.pid == pid && p.active == "removed") {
                p.active = "running";
                existing = true;
                break;
            }
        }
        if (!existing) {
            processes.push_back({ pid, processSize, "running", 1 }); // Record process information
        }
        numPagedIn++;
        availableMemory -= processSize;
        return true;
    } 
    
    if (!isAllRunning()) { // If allocation fails, reallocate by removing oldest process
        deallocateOldest();
        return flatAllocate(pid, processSize);
    }

    return false;
}

// Paging memory allocation
bool MemoryManager::pagingAllocate(int pid, int processSize) {
    int requiredFrames = (processSize + frameSize - 1) / frameSize; // Calculate the number of frames needed (ceil division)
    int freeFrames = 0;
    int frameCtr = 0;
    std::vector<int> listOfFrames;

    if (isAllocatedIdle(pid)) {
        std::cout << "already allocated process and idle " << pid << std::endl;
        setStatus(pid, "running");
        return true;
    }

    for (int i = 0; i < memory.size(); ++i) {
        if (memory[i] == -1) { // Frame is free
            listOfFrames.push_back(i);
            ++freeFrames;
            if (freeFrames == requiredFrames) { // Enough frames found
                for (int j = 0; j < memory.size(); ++j) {
                    if (listOfFrames[frameCtr] == j) {
                        memory[j] = pid; // Allocate frames to the process
                        frameCtr++;
                        if (frameCtr == listOfFrames.size()) {
                            break;
                        }
                    }
                }

                bool existing = false;
                for (auto& p : processes) {
                    if (p.pid == pid && p.active == "removed") {
                        p.active = "running";
                        existing = true;
                        break;
                    }
                }
                if (!existing) {    // Process ID, Process Size, Allocation status, time
                    processes.push_back({ pid, processSize, "running", 1 }); // Record process information
                }
                
                availableMemory -= processSize;
                numPagedIn += requiredFrames;
                return true; // Allocation successful
            }
            //std::cout << "total now " << i << std::endl;
        }
    }

    // If allocation fails, reallocate by removing oldest process
    if (!isAllRunning() && freeFrames != requiredFrames) {
        deallocateOldest();
        return pagingAllocate(pid, processSize);
    }
    return false;
}

// Returns if process is already in the memory or not
bool MemoryManager::isAllocated(int pid) {
    bool allocated = false;
    for (auto& p : processes) {
        if (p.pid == pid && p.active == "removed") {
            return false;
        }

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
    int count = 0;
    int numCtr = 0;
    for (auto& p : processes) {
        count++;
        if (p.active == "running") {
            numCtr++;
        }
        if (p.active == "removed") {
            count--;
        }
    }
    return (count == numCtr);
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

    for (auto& p : processes) {
        if (p.time > highest && p.active == "idle") {
            highest = p.time;
            oldestProcess = p.pid;
        }
    }

    if (oldestProcess != 0) {
        bs.storeProcess(oldestProcess);     //backing store
        deallocateMemory(oldestProcess);    //deallocate now
    }
}

// Deallocate memory when the process finishes
void MemoryManager::deallocateMemory(int pid) {
    int freedMemory = 0;

    if (memType == "flat") {
        for (auto& p : processes) {
            if (p.pid == pid) {
                freedMemory = p.memory;
                numPagedOut++;
                break;
            }
        }
    }
    else {
        for (int i = 0; i < memory.size(); ++i) {
            if (memory[i] == pid) {
                memory[i] = -1;
                freedMemory += frameSize;
                numPagedOut++;
            }
        }
    }
   
    availableMemory += freedMemory;

    // Remove the process from the active list
    for (auto& p : processes) {
        if (p.pid == pid) {
            p.active = "removed";
            break;
        }
    }
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

void MemoryManager::printMemoryDetails(float cpuUtil) {
    std::cout << "----------------------------------------------" << std::endl;
    std::cout << "| PROCESS-SMI v01.00   Driver Version: 01.00 |" << std::endl;
    std::cout << "----------------------------------------------" << std::endl;
    std::cout << "CPU-Util: " << cpuUtil << "%" << std::endl;
    std::cout << "Memory Usage: " << getUsedMemory() << "MiB / " << getMaxMemory() << "MiB" << std::endl;
    std::cout << "Memory Util: " << getMemoryUtil() << "%" << std::endl << std::endl;
    std::cout << "==============================================" << std::endl;
    std::cout << "Running processes and memory usage:" << std::endl;
    std::cout << "----------------------------------------------" << std::endl;
    for (const auto& p : processes) {
        if (p.active == "running" || p.active == "idle") {
            std::cout << p.pid << "\t" << p.memory << "MiB" << std::endl;
        }
    }
    std::cout << "----------------------------------------------" << std::endl << std::endl;
}

int MemoryManager::getPagedIn() const {
    return numPagedIn;
}
int MemoryManager::getPagedOut() const {
    return numPagedOut;
}