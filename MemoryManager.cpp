#include "MemoryManager.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <fstream>
#include <ctime>
#include <iostream>

// Constructor: Initialize memory with -1 (indicating free space)
MemoryManager::MemoryManager(int maxMemory, int memPerPoc, int frameSize, int availableMemory) : memory(maxMemory / memPerPoc, -1) {
    this->maxMemory = maxMemory;
    this->memPerProc = memPerPoc;
    this->frameSize = frameSize;
    this->availableMemory = availableMemory;
}

// First-fit memory allocation
bool MemoryManager::allocateMemory(int pid) {
    int block = memPerProc;
    int maxBlocks = maxMemory / block;
<<<<<<< Updated upstream
 
=======

>>>>>>> Stashed changes
    // First-fit algorithm to find the first block of free memory
    for (int i = 0; i < maxBlocks; ++i) {
        if (memory[i] == -1) {
            memory[i] = pid;
<<<<<<< Updated upstream
            
            processes.push_back({ pid, i * block, (i + 1) * block - 1, true });
=======

            processes.push_back({ pid, i * block, (i + 1) * block - 1, true, false });
>>>>>>> Stashed changes
            availableMemory -= block;
            return true;
        }
    }

    // If allocation fails, calculate external fragmentation
<<<<<<< Updated upstream
    totalFragmentation = maxMemory - block;
    return false;
=======
    int total = 0;
    int largest = 0;
    int current = 0;

    for (int i = 0; i < maxBlocks; ++i) {
        if (memory[i] == -1) {
            total += block;
            current += block;
        }
        else {
            if (current > largest) {
                largest = current;
            }
            current = 0;
        }
    }

    if (current > largest) {
        largest = current;
    }

    totalFragmentation += (total - largest);
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
>>>>>>> Stashed changes
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

<<<<<<< Updated upstream
// Print memory layout, timestamp, process details, and fragmentation status
=======
// Mark a process as idle when not running
void MemoryManager::setIdle(int pid, bool idle) {
    for (auto& p : processes) {
        if (p.pid == pid) {
            p.isIdle = idle;
            break;
        }
    }
}

// Print memory layout every quantum cycle
>>>>>>> Stashed changes
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

// Get the number of active processes
int MemoryManager::getActiveProcessesCount() const {
    int count = 0;
    for (const auto& p : processes) {
        if (p.active) ++count;
    }
    return count;
}

// Get the current timestamp in a human-readable format
std::string MemoryManager::getCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    time_t currentTime = std::chrono::system_clock::to_time_t(now);
    struct tm buf;
    localtime_s(&buf, &currentTime);

    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "%m/%d/%Y %I:%M:%S %p", &buf);

    return timeStr;
}

int MemoryManager::getAvailableMemory() const {
    return availableMemory;
}

void MemoryManager::setAvailableMemory(int available) {
    availableMemory = available;
}