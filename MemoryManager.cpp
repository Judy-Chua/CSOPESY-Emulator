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

    // First-fit algorithm to find the first block of free memory
    for (int i = 0; i < maxBlocks; ++i) {
        if (memory[i] == -1) {
            memory[i] = pid;
            processes.push_back({ pid, i * block, (i + 1) * block - 1, true });
            availableMemory -= block;
            return true;
        }
    }

    // If allocation fails, calculate external fragmentation
    totalFragmentation += block;
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