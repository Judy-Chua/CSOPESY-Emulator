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

// allocate based on type
bool MemoryManager::allocate(int pid, int processSize) {
    for (auto& p : processes) {
        if (p.active)
            p.time++;
    }

    if (memType == "flat") {
        return flatAllocate(pid, processSize);
    }

    return pagingAllocate(pid, processSize);   
}

// First-fit memory allocation
bool MemoryManager::flatAllocate(int pid, int processSize) {
    // First-fit algorithm to find the first block of free memory
    if (availableMemory > processSize) {
        processes.push_back({ pid, processSize, true, 1 }); // Record process information
        return true;
    } else { // If allocation fails, reallocate by removing oldest process
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

                processes.push_back({ pid, processSize, true, 1 }); // Record process information
                    // Process ID, Process Size, Allocation status, time
                return true; // Allocation successful
            }
        }
    }

    // If allocation fails, reallocate by removing oldest process
    if (freeFrames != requiredFrames) {
        deallocateOldest();
        pagingAllocate(pid, processSize);
    }
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

void MemoryManager::deallocateOldest() {
    int highest = 0;
    int oldestProcess = 0;

    for (auto& p : processes) {
        if (p.time > highest) {
            highest = p.time;
            oldestProcess = p.pid;
        }
    }

    //backing store code here (already existing in mco2-draft)
    
    deallocateMemory(oldestProcess);
}

// Deallocate memory when the process finishes
void MemoryManager::deallocateMemory(int pid) {
    int freedMemory = 0;
    for (int i = 0; i < memory.size(); ++i) {
        if (memory[i] == pid) {
            memory[i] = -1;
            freedMemory += frameSize;
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
