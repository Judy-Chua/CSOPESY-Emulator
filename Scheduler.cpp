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
    process->setState(Process::READY); //set to READY first
    processes.push_back(process);
    processQueue.push(process);
    cv.notify_all();
}

void Scheduler::startScheduling() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (delaysPerExec >= 0 && delaysPerExec < 50) delaysPerExec = 50;
    stop = false;
    if (!schedulerThread.joinable()) {
        schedulerThread = std::thread(&Scheduler::schedule, this);
    }
}

void Scheduler::generateProcesses() {
    if (!generateProcessThread.joinable()) {
        generateProcessThread = std::thread([this]() {
            while (!stop) {
                for (int i = 0; i < batchFreq; i++) {
                    generateProcess();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(delaysPerExec));
            }
        });
    }
}

void Scheduler::startTicksProcesses() {
    if (!ticksThread.joinable()) {
        ticksThread = std::thread(&Scheduler::startTicks, this);
    }
}

void Scheduler::stopScheduler() {
    {
        stop = true;
    }
    if (generateProcessThread.joinable()) {
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
    int random = generateRandomNumber(minIns, maxIns);
    int processMemory = generateMemory();
    int lastPID = ConsoleManager::getInstance()->getCurrentPID() + 1;
    string initialName = "P";
    string newName = initialName + to_string(lastPID);
    ConsoleManager::getInstance()->createProcess(newName, random, processMemory);
}

void Scheduler::scheduleFCFS() {
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
                }
                coreAvailable[coreId] = false;
                assigned = true;
                processQueue.pop();

                if (workers[coreId].joinable()) {
                    workers[coreId].join();
                }

                workers[coreId] = std::thread(&Scheduler::worker, this, coreId, process);
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
                }
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
            std::this_thread::sleep_for(chrono::milliseconds(delaysPerExec));
        }
        memoryManager.setStatus(process->getPID(), "idle");
        coreAvailable[coreId] = true;   //set to true now since done

        if (!process->isFinished()) {
            process->setState(Process::WAITING);
            process->setCoreID(-1);
            processQueue.push(process);
        }
        else {
            memoryManager.deallocateMemory(process->getPID());
        }
        cv.notify_all(); // Notify scheduler of available core
    }
}

void Scheduler::worker(int coreId, std::shared_ptr<Process> process) {
    if (process == nullptr) {
        return;
    }
    if (process != nullptr && !process->getName().empty()) {
        process->setState(Process::RUNNING);
        process->setCoreID(coreId);

        while (!process->isFinished() && process->getState() != Process::WAITING) {
            process->executeCommand(coreId);
            incrementTicks(1);
            
            std::this_thread::sleep_for(chrono::milliseconds(delaysPerExec));
        }
        coreAvailable[coreId] = true;   //set to true now since done
        memoryManager.deallocateMemory(process->getPID());
        cv.notify_all(); 
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
    shortcut << "CPU Utilization: " << getCpuUtilization() << "%" << std::endl;
    shortcut << "Cores used: " << getUsedCores() << std::endl;
    shortcut << "Cores available: " << countAvailCores() << std::endl << std::endl;
    shortcut << "--------------------------------------------------\n";
    shortcut << "Running processes:\n";

    {
        for (const auto& process : processes) {
            if (process->getState() == Process::RUNNING && process->getCoreID() != -1) {
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
    float cpuUtil = getCpuUtilization();
    memoryManager.printMemoryDetails(cpuUtil);
}

void Scheduler::printVmstat() {
    long long currentIdle = idleTicks;
    long long currentActive = activeTicks;
    std::cout << makeSpaces(memoryManager.getMaxMemory()) << " K total memory" << std::endl;
    std::cout << makeSpaces(memoryManager.getUsedMemory()) << " K used memory" << std::endl;
    std::cout << makeSpaces(memoryManager.getAvailableMemory()) << " K free memory" << std::endl;
    std::cout << makeSpacesTicks(currentIdle) << " K idle cpu ticks" << std::endl;
    std::cout << makeSpacesTicks(currentActive) << " K active cpu ticks" << std::endl;
    std::cout << makeSpacesTicks(currentIdle + currentActive) << " K total cpu ticks" << std::endl;
    std::cout << makeSpaces(memoryManager.getPagedIn()) << " K num paged in" << std::endl;
    std::cout << makeSpaces(memoryManager.getPagedOut()) << " K num paged out" << std::endl << std::endl;
}

int Scheduler::countAvailCores() {
    int count = 0;
    if (type == "rr") {
        {
            for (int coreId = 0; coreId < numCores; ++coreId) {
                if (coreAvailable[coreId]) { // Check if core is marked as available
                    count++;
                }
            }
        }
    }
    else { // fcfs
        count = numCores;
        {
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
    int avail = countAvailCores();
    int used = numCores - avail;
    return used;
}

float Scheduler::getCpuUtilization() {
    auto used = getUsedCores();
    float percent = float(used) / numCores * 100;
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

std::string Scheduler::makeSpacesTicks(long long input) {
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
            if (coreAvailable[coreId] == true) {
                idleTicks++;
            }
        }
        std::this_thread::sleep_for(chrono::milliseconds(10));
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