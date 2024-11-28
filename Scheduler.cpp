#include "ConsoleManager.h"
#include "Scheduler.h"
#include "Process.h"
#include "MemoryManager.h"
#include <fstream>
#include <iostream>
#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>
#include <memory>  
#include <random>

using namespace std;

Scheduler::Scheduler(int numCores, const std::string& type, int timeSlice, int freq, int min, int max, int delay, int memMax, int memFrame, int minMemProc, int maxMemProc) :
    numCores(numCores), type(type), coreAvailable(numCores, true), workers(numCores),
    timeSlice(timeSlice), batchFreq(freq), minIns(min), maxIns(max), delaysPerExec(delay),
    maxOverallMem(memMax), memPerFrame(memFrame), minMemPerProc(minMemProc), maxMemPerProc(maxMemProc),
    memoryManager(memMax, memFrame, memMax), activeTicks(0), idleTicks(0) {}

void Scheduler::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(queueMutex);
    process->setState(Process::READY); //set to READY first
    processes.push_back(process);
    processQueue.push(process);
    std::cout << "Process added to queue: " << process->getName() << std::endl;
    cv.notify_all();
}

void Scheduler::startScheduling() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (delaysPerExec == 0) delaysPerExec = 10;
    stop = false;
    if (!schedulerThread.joinable()) {
        schedulerThread = std::thread(&Scheduler::schedule, this);
    }
    if (!ticksThread.joinable()) {
        ticksThread = std::thread(&Scheduler::startTicks, this);
    }
}

void Scheduler::generateProcesses() {
    if (!generateProcessThread.joinable()) {
        std::cout << "starting generateProcessThread";
        generateProcessThread = std::thread(&Scheduler::generateProcess, this);
    }
}

void Scheduler::stopScheduler() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stop = true;
    }
    if (generateProcessThread.joinable()) {
        std::cout << "ending generateProcessThread";
        generateProcessThread.join();
    }
    cv.notify_all();
}

void Scheduler::schedule() {
    while (true) {
        if (type == "fcfs") {
            scheduleFCFS();
        }
        else if (type == "rr") {
            scheduleRR();
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void Scheduler::generateProcess() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (stop) {
                std::cout << "something triggered here " << std::endl;
                break;  // Exit if stop is true
            }
        }
        
        std::cout << "cpu now " << getActiveTicks() << " batchfreq " << batchFreq << std::endl;
        if (getActiveTicks() % batchFreq == 0) {
            int random = generateRandomNumber(minIns, maxIns);
            int processMemory = generateMemory();
            int lastPID = ConsoleManager::getInstance()->getCurrentPID() + 1;
            string initialName = "P";
            string newName = initialName + to_string(lastPID);
            std::cout << "Creating new process: " << newName << std::endl;
            ConsoleManager::getInstance()->createProcess(newName, random, processMemory);
        }
        this_thread::sleep_for(std::chrono::milliseconds(1));
        
    }
}

void Scheduler::scheduleFCFS() {
    while (true) {
        std::shared_ptr<Process> process = nullptr;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this] {
                return !processQueue.empty();
                });

            if (processQueue.empty()) {
                break;
            }

            process = processQueue.front();
        }

        bool assigned = false;
        for (int coreId = 0; coreId < numCores; ++coreId) {
            if (memoryManager.getAvailableMemory() > 0 || memoryManager.isAllocated(process->getPID())) { // has available memory
                if (coreAvailable[coreId]) {
                    process->setState(Process::RUNNING);
                    process->setCoreID(coreId);
                    coreAvailable[coreId] = false;

                    processQueue.pop();

                    if (workers[coreId].joinable()) {
                        workers[coreId].join();
                    }

                    workers[coreId] = std::thread(&Scheduler::worker, this, coreId, process);
                    assigned = true;
                    break;
                }
            }                
        }

        if (!assigned) {
            processQueue.pop();
            processQueue.push(process);
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this] {
                return std::any_of(coreAvailable.begin(), coreAvailable.end(), [](bool available) { return available; });
                });
        }
    }

    cv.notify_all(); // Notify all waiting threads in case of a stop
}

void Scheduler::scheduleRR() {
    std::unique_lock<std::mutex> lock(queueMutex);
    while (true) {
        cv.wait(lock, [this] {
            return !processQueue.empty(); // Wait until a process is available in the queue
            });

        std::shared_ptr<Process> process = processQueue.front();
        bool assigned = false;

        // Assign process to an available core but check first if it has available memory or already in memory
        for (int coreId = 0; coreId < numCores; ++coreId) {
            if (coreAvailable[coreId]) {                                //check if allocated
                if (!memoryManager.isAllocated(process->getPID())) {    //check if it can be allocated
                    if (!memoryManager.allocate(process)) {
                        //cannot be allocated
                        continue;
                    }
                    std::cout << "***Allocated " << process->getPID() << std::endl;  
                }
                std::cout << "Assigning to Core " << coreId << " for process " << process->getPID() << std::endl << std::endl;
                coreAvailable[coreId] = false;
                assigned = true;
                processQueue.pop();

                if (workers[coreId].joinable()) {
                    workers[coreId].join();
                }

                workers[coreId] = std::thread(&Scheduler::workerRR, this, coreId, process);
                break;
            }
        }

        if (!assigned) { //no memory or core so go back
            processQueue.pop();
            processQueue.push(process);
            cv.wait(lock, [this] {
                return std::any_of(coreAvailable.begin(), coreAvailable.end(), [](bool available) { return available; });
                });
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    lock.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

// Worker function specific to RR scheduling
void Scheduler::workerRR(int coreId, std::shared_ptr<Process> process) {
    if (process == nullptr) {
        return;
    }
    if (process != nullptr && !process->getName().empty()) {
        process->setState(Process::RUNNING);
        process->setCoreID(coreId);
        int ctr = 0;
        
        // Execute process within the time slice for RR
        while (ctr < timeSlice && !process->isFinished()) {
            process->executeCommand(coreId);
            ctr++;
            incrementTicks(1);
            if (getActiveTicks() % timeSlice == 0 && getActiveTicks() != 0) {
                std::cout << "cpu here is " << getActiveTicks() << std::endl;
                memoryManager.printMemoryLayout(getActiveTicks() / timeSlice);
            }

            std::this_thread::sleep_for(chrono::milliseconds(delaysPerExec));
        }
        
        coreAvailable[coreId] = true;   //set to true now since done
        if (!process->isFinished()) {
            process->setState(Process::WAITING);
            process->setCoreID(-1);
            memoryManager.setStatus(process->getPID(), "idle");
            processQueue.push(process);
        }
        else {
            memoryManager.deallocateMemory(process->getPID());
        }
        cv.notify_all(); // Notify scheduler of available core
    }
}

void Scheduler::worker(int coreId, std::shared_ptr<Process> process) {
    if (process != nullptr && !process->getName().empty()) {
        process->setStartTime();

        memoryManager.allocate(process);
        // Process execution
        while (!process->isFinished() && process->getState() != Process::WAITING) {
            process->executeCommand(coreId);
            std::this_thread::sleep_for(std::chrono::duration<int>(delaysPerExec));
        }

        memoryManager.deallocateMemory(process->getPID());
        coreAvailable[coreId] = true;

        cv.notify_all();  // Notify scheduler that a core is now available
    }
}

void Scheduler::printActiveScreen() {
    screenInfo(std::cout);
}

void Scheduler::reportUtil() {
    std::ofstream outFile("csopesy-log.txt");
    if (outFile.is_open()) {
        screenInfo(outFile);
        outFile.close();
    }
    else {
        std::cerr << "Error: Unable to open log file.\n";
    }
}

void Scheduler::screenInfo(std::ostream& shortcut) {
    int runCtr = 0, finCtr = 0;
    std::cout << "inside screenInfo to call getCpuUtilization " << std::endl;
    shortcut << "CPU Utilization: " << getCpuUtilization() << "%" << std::endl;
    shortcut << "Cores used: " << getUsedCores() << std::endl;
    shortcut << "Cores available: " << countAvailCores() << std::endl << std::endl;
    shortcut << "--------------------------------------------------\n";
    shortcut << "Running processes:\n";

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (const auto& process : processes) {
            if (process->getState() != Process::FINISHED && process->getCoreID() != -1) {
                shortcut << process->getName() << "\tStarted: " << process->getStartTime()
                    << "   Core: " << process->getCoreID() << "   " << process->getCommandCounter() << " / "
                    << process->getLinesOfCode() << std::endl;
                runCtr++;
            }
        }
    }

    if (runCtr == 0) {
        shortcut << "    No running processes.\n";
    }
    shortcut << "\nFinished processes:\n";

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (const auto& process : processes) {
            std::lock_guard<std::mutex> processLock(process->processMutex);
            if (process->getState() == Process::FINISHED) {
                shortcut << process->getName() << "\tEnded: " << process->getEndTime()
                    << "   Finished " << process->getCommandCounter()
                    << " / " << process->getLinesOfCode() << std::endl
                    << "\t\tStarted: " << process->getStartTime()
                    << "   Core: " << process->getCoreID() << std::endl;
                finCtr++;
            }
        }
    }

    if (finCtr == 0) {
        shortcut << "    No finished processes.\n";
    }
    shortcut << "--------------------------------------------------\n\n";
}

int Scheduler::generateRandomNumber(int minIns, int maxIns) {
    random_device random;
    mt19937 generate(random());
    uniform_int_distribution<> distr(minIns, maxIns);
    return distr(generate);
}

int Scheduler::generateInstructions() {
    random_device random;
    mt19937 generate(random());
    uniform_int_distribution<> distr(minIns, maxIns);
    return distr(generate);
}

int Scheduler::generateMemory() {
    int minExp = static_cast<int>(std::log2(minMemPerProc));
    int maxExp = static_cast<int>(std::log2(maxMemPerProc));
    int randomExp = generateRandomNumber(minExp, maxExp);
    return 1 << randomExp;
}

void Scheduler::printProcessSMI() {
    //std::lock_guard<std::mutex> lock(queueMutex);
    std::cout << "----------------------------------------------" << std::endl;
    std::cout << "| PROCESS-SMI vxx.xx Driver Version: xx.xx |" << std::endl;
    std::cout << "----------------------------------------------" << std::endl;
    std::cout << "CPU-Util: " <<  getCpuUtilization() << "%" << std::endl;
    std::cout << "Memory Usage: " << memoryManager.getUsedMemory() << "MiB / " << memoryManager.getMaxMemory() << "MiB" << std::endl;
    std::cout << "Memory Util: " << memoryManager.getMemoryUtil() << "%" << std::endl << std::endl;
    std::cout << "==============================================" << std::endl;
    std::cout << "Running processes and memory usage:" << std::endl;
    std::cout << "----------------------------------------------" << std::endl;
    std::cout << "pXXXX" <<  "\t" << "xxx" << "MiB" << std::endl;

    /*{
        std::lock_guard<std::mutex> lock(queueMutex);
        for (const auto& process : processes) {
            if (process->getState() != Process::FINISHED && process->getCoreID() != -1) {
                std::cout << process->getName() << "\t" << process->getMemorySize() << "MiB" << std::endl;
            }
        }
    }*/

    std::cout << "----------------------------------------------" << std::endl << std::endl;
}

void Scheduler::printVmstat() {
    std::lock_guard<std::mutex> lock(queueMutex);
    std::cout << makeSpaces(memoryManager.getMaxMemory()) << " K total memory" << std::endl;
    std::cout << makeSpaces(memoryManager.getUsedMemory()) << " K used memory" << std::endl;
    std::cout << makeSpaces(memoryManager.getAvailableMemory()) << " K free memory" << std::endl;
    std::cout << makeSpaces(idleTicks) << " K idle cpu ticks" << std::endl;
    std::cout << makeSpaces(activeTicks) << " K active cpu ticks" << std::endl;
    std::cout << makeSpaces(idleTicks + activeTicks) << " K total cpu ticks" << std::endl;
    std::cout << makeSpaces(memoryManager.getPagedIn()) << " K num paged in" << std::endl;
    std::cout << makeSpaces(memoryManager.getPagedOut()) << " K num paged out" << std::endl << std::endl;
}

int Scheduler::countAvailCores() {
    std::cout << "inside countAvailCores " << std::endl;
    int count = 0;
    if (type == "rr") {
        std::cout << "rr " << std::endl;
        // count = 0;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            std::cout << "locking then looping cores " << std::endl;
            for (int coreId = 0; coreId < numCores; ++coreId) {
                std::cout << "checking core " << coreId << std::endl;
                if (coreAvailable[coreId]) { // Check if core is marked as available
                    count++;
                }
            }
            std::cout << "final count " << count << std::endl;
        }
    }
    else { // fcfs
        count = numCores;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            for (const auto& process : processes) { //more updated version instead of checking coresAvailable
                if (process->getState() == Process::RUNNING && process->getCoreID() != -1) {
                    count--;
                }
            }
        }
    }
    return count;
}

int Scheduler::getUsedCores() {
    std::cout << "inside getUsedCores to call countAvailCores " << std::endl;
    int avail = countAvailCores();
    std::cout << "got the available cores " << avail << std::endl;
    int used = numCores - avail;
    std::cout << "returning used " << used << std::endl;
    return used;
}

float Scheduler::getCpuUtilization() {
    std::cout << "inside getCpuUtilization to call getUsedCores " << std::endl;
    auto used = getUsedCores();
    std::cout << "got the used cores " << used << std::endl;
    float percent = float(used) / numCores * 100;
    std::cout << "returning percent " << percent << std::endl;
    return percent;
}

std::string Scheduler::makeSpaces(int input) {
    int origLength = std::to_string(input).length();
    std::string newString;
    for (int i = 0; i < 10 - origLength; i++) {
        newString += " ";
    }

    newString += std::to_string(input);

    return newString;
}

void Scheduler::startTicks() {
    while (true) {
        for (int coreId = 0; coreId < numCores; ++coreId) {
            if (coreAvailable[coreId] == false) {
                idleTicks++;
            }
        }
        std::this_thread::sleep_for(chrono::milliseconds(delaysPerExec));
    }
}

long long Scheduler::getActiveTicks() {
    std::lock_guard<std::mutex> lock(cpuMutex);
    return activeTicks;
}

void Scheduler::incrementTicks(long long ticks) {
    std::lock_guard<std::mutex> lock(cpuMutex);
    activeTicks += ticks;
}

long long Scheduler::getIdleTicks() {
    std::lock_guard<std::mutex> lock(cpuMutex);
    return idleTicks;
}

void Scheduler::incrementIdleTicks(long long ticks) {
    std::lock_guard<std::mutex> lock(cpuMutex);
    idleTicks += ticks;
}