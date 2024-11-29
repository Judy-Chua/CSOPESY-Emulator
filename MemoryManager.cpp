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
        //std::cout << "already allocated process and idle " << pid << std::endl;
        setStatus(pid, "running");
        return true;
    }
    
    if (availableMemory >= processSize) {
        //std::cout << "Allocating process since free " << pid << std::endl;
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
        //std::cout << "available memory now is " << availableMemory << std::endl;
        return true;
    } 
    
    if (!isAllRunning()) { // If allocation fails, reallocate by removing oldest process
        //std::cout << "deallocate process " << pid << std::endl;
        deallocateOldest();
        return flatAllocate(pid, processSize);
    }

    //std::cout << "all processes are running " << pid << std::endl;
    return false;
}

// Paging memory allocation
bool MemoryManager::pagingAllocate(int pid, int processSize) {
    int requiredFrames = (processSize + frameSize - 1) / frameSize; // Calculate the number of frames needed (ceil division)
    //std::cout << "paging allocate required frames " << requiredFrames << std::endl;
    int freeFrames = 0;
    int frameCtr = 0;
    std::vector<int> listOfFrames;
    //std::cout << "memory size " << memory.size() << std::endl;

    if (isAllocatedIdle(pid)) {
        std::cout << "already allocated process and idle " << pid << std::endl;
        setStatus(pid, "running");
        return true;
    }

    for (int i = 0; i < memory.size(); ++i) {
        if (memory[i] == -1) { // Frame is free
            //std::cout << "free frame " << i << std::endl;
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
                
                //std::cout << "allocated in memory " << pid << std::endl;
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
            //std::cout << "set status " << p.pid << " to " << status << std::endl;
            p.active = status;
            //std::cout << "done " << p.active << std::endl;
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
        //std::cout << "backing store " << oldestProcess << std::endl;
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
    //std::cout << "deallocated " << pid << " " << freedMemory << " so " << availableMemory << std::endl;

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