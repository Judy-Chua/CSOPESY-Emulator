#include "MemoryManager.h"
#include <iostream>
#include <sstream>

// Constructor: Initialize memory with -1 (indicating free space)
MemoryManager::MemoryManager(int maxMemory, int memPerPoc, int frameSize, int availableMemory) : memory(maxMemory / frameSize, -1) {
    this->maxMemory = maxMemory;
    this->memPerProc = memPerPoc;
    this->frameSize = frameSize;
    this->availableMemory = availableMemory;
}

// First-fit memory allocation
bool MemoryManager::allocateMemory(int pid) {
    int requiredFrames = memPerProc / frameSize;
    int freeFrames = 0;
    int start = -1;

    // First-fit algorithm to find the first block of free memory
    for (int i = 0; i < memory.size(); ++i) {
        if (memory[i] == -1) {
            if (start == -1)
                start = i;
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
    totalFragmentation = maxMemory - (processes.size() * memPerProc);
    return false;
}

// Deallocate memory when the process finishes
void MemoryManager::deallocateMemory(int pid) {
    for (int i = 0; i < memory.size(); ++i) {
        if (memory[i] == pid) {
            memory[i] = -1;
        }
    }

    // Remove the process from the active list
    for (auto& p : processes) {
        if (p.pid == pid) {
            p.active = false;
        }
    }
}

// Print memory layout, timestamp, process details, and fragmentation status
void MemoryManager::printMemoryLayout(int cycle) const {
    std::ofstream file("memory_stamp_" + std::to_string(cycle) + ".txt");
    file << "Timestamp: " << getCurrentTime() << "\n";
    file << "Number of processes in memory: " << getActiveProcessesCount() << "\n";
    file << "Total external fragmentation in KB: " << totalFragmentation / 1024 << "\n\n";

    file << "----end---- = " << maxMemory << "\n";
    for (const auto& p : processes) {
        if (p.active) {
            file << p.memoryEnd << "\n";
            file << "P" << p.pid << "\n";
            file << p.memoryStart << "\n";
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
    std::time_t now = std::time(nullptr);
    std::tm localtime;
    std::ostringstream oss;
    oss << std::put_time(&localtime, "%m/%d/%Y %I:%M:%S %p");
    return oss.str();
}

int MemoryManager::getAvailableMemory() const {
    return availableMemory;
}

void MemoryManager::setAvailableMemory(int available) {
    availableMemory = available;
}