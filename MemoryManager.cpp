#include "MemoryManager.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <fstream>
#include <ctime>
// Constructor: Initialize memory with -1 (indicating free space)
MemoryManager::MemoryManager(int maxMemory, int memPerProc, int frameSize, int availableMemory)
    : memory(maxMemory / memPerProc, -1), maxMemory(maxMemory), memPerProc(memPerProc),
    frameSize(frameSize), availableMemory(availableMemory) {}

// First-fit memory allocation
bool MemoryManager::allocateMemory(int pid) {
    int block = memPerProc;
    int maxBlocks = maxMemory / block;

    for (int i = 0; i < maxBlocks; ++i) {
        if (memory[i] == -1) {
            memory[i] = pid;
            processes.push_back({ pid, i * block, (i + 1) * block - 1, true, false });
            availableMemory -= block;
            return true;
        }
    }

    // If memory is full, move the oldest process to the ready queue
    for (auto& p : processes) {
        if (!p.isRunning && p.active) { // Check for idle processes
            deallocateMemory(p.pid);
            return allocateMemory(pid);
        }
    }

    return false; // Memory allocation failed
}

// Deallocate memory only if the process is finished
void MemoryManager::deallocateMemory(int pid) {
    for (auto& p : processes) {
        if (p.pid == pid && !p.isRunning) {
            int block = memPerProc;
            int maxBlocks = maxMemory / block;

            for (int i = 0; i < maxBlocks; ++i) {
                if (memory[i] == pid) {
                    memory[i] = -1;
                    availableMemory += block;
                }
            }
            p.active = false; // Mark process as deallocated
            return;
        }
    }
}

// Mark a process as idle when not running
void MemoryManager::markIdle(int pid) {
    for (auto& p : processes) {
        if (p.pid == pid && p.active) {
            p.isRunning = false;
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
    file << "\n----start---- = 0\n";
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

