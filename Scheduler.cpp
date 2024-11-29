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
    //std::cout << "in add process " << process->getPID() << std::endl;
    //std::lock_guard<std::mutex> lock(queueMutex);
    //std::cout << "no more locking " << process->getName() << std::endl;
    process->setState(Process::READY); //set to READY first
    //std::cout << "set state " << process->getName() << std::endl;
    processes.push_back(process);
    //std::cout << "Process added to list: " << process->getName() << std::endl;
    processQueue.push(process);
    //std::cout << "Process added to queue: " << process->getName() << std::endl;
    cv.notify_all();
    //std::cout << "Process added: " << process->getName() << std::endl;

}

void Scheduler::startScheduling() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (delaysPerExec >= 0 && delaysPerExec < 100) delaysPerExec = 100;
    if (batchFreq >= 10) batchFreq /= 10;
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
        //std::lock_guard<std::mutex> lock(queueMutex);
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
    //std::cout << "cpu now " << getActiveTicks() << " batchfreq " << batchFreq << std::endl;
    int random = generateRandomNumber(minIns, maxIns);
    int processMemory = generateMemory();
    int lastPID = ConsoleManager::getInstance()->getCurrentPID() + 1;
    string initialName = "P";
    string newName = initialName + to_string(lastPID);
    //std::cout << "Creating new process: " << newName << std::endl;
    ConsoleManager::getInstance()->createProcess(newName, random, processMemory);
    //std::cout << "console manageris done " << newName << std::endl;
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

void Scheduler::printProcessQueue() {
    std::queue<std::shared_ptr<Process>> tempQueue = processQueue; // Create a copy of the queue

    std::cout << "Current Process Queue: ";
    if (tempQueue.empty()) {
        std::cout << "The queue is empty.\n";
        return;
    }

    while (!tempQueue.empty()) {
        auto process = tempQueue.front();
        tempQueue.pop();

        // Print process details
        std::cout << process->getName() << std::endl;
    }
}
void Scheduler::printRunningProcesses() {
    std::cout << "Current Running Processes:\n";
    bool anyRunning = false;

    // Iterate through all processes and check for running state
    for (const auto& process : processes) {
        if (process->getState() == Process::RUNNING) {
            std::cout << ", PID: " << process->getPID()
                << ", Core: " << process->getCoreID()
                << "\n";
            anyRunning = true;
        }
    }

    if (!anyRunning) {
        std::cout << "No processes are currently running.\n";
    }
}
void Scheduler::scheduleRR() {
    std::unique_lock<std::mutex> lock(queueMutex);

    while (true) {
        // Wait until there is at least one process in the queue
        cv.wait(lock, [this] {
            return !processQueue.empty();
            });

        // Get the process at the front of the queue
        std::shared_ptr<Process> process = processQueue.front();
        processQueue.pop(); // Remove it from the queue

        bool assigned = false;

        // Try to assign the process to an available core
        for (int coreId = 0; coreId < numCores; ++coreId) {
            if (coreAvailable[coreId]) {
                coreAvailable[coreId] = false;
               // std::cout << "\nAssigning to Core " << coreId << " process " << process->getPID() << std::endl;

                // Check if the process is allocated in memory
                if (!memoryManager.isAllocated(process->getPID())) {
                    if (!memoryManager.allocate(process)) {
                        // Memory allocation failed; re-add the process to the queue and break
                        processQueue.push(process);
                        coreAvailable[coreId] = true;
                        break;
                    }
                }

                // Mark the core as unavailable and assign the process
                process->setState(Process::RUNNING);
                process->setCoreID(coreId);

                // Start the worker thread for this core
                if (workers[coreId].joinable()) {
                    workers[coreId].join();
                }
                workers[coreId] = std::thread(&Scheduler::workerRR, this, coreId, process);

                assigned = true;
                //printRunningProcesses();
                break; // Break out of the core assignment loop
            }
        }

        if (!assigned) {
            // If no core is available, re-add the process to the queue and wait
            processQueue.push(process);
            cv.wait(lock, [this] {
                return std::any_of(coreAvailable.begin(), coreAvailable.end(), [](bool available) { return available; });
                });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent excessive CPU usage
    }
}


// Worker function specific to RR scheduling
void Scheduler::workerRR(int coreId, std::shared_ptr<Process> process) {
    if (process == nullptr) {
        return;
    }
    if (process != nullptr && !process->getName().empty()) {
        int ctr = 0;
        //std::cout << "working process " << process->getPID() << " cpu " << coreId << std::endl;
        // Execute process within the time slice for RR
        while (ctr < timeSlice && !process->isFinished()) {
            process->executeCommand(coreId);
            ctr++;
            incrementTicks(1);

            //std::thread([this]() { this->generateProcess(); }).detach();

            if (getActiveTicks() % timeSlice == 0 && getActiveTicks() != 0) {
                
                memoryManager.printMemoryLayout(getActiveTicks() / timeSlice);
            }
            //std::cout << "executing process " << getActiveTicks() << std::endl;
            std::this_thread::sleep_for(chrono::milliseconds(delaysPerExec));
        }
        std::unique_lock<std::mutex> lock(queueMutex);
        if (!process->isFinished()) {
            process->setState(Process::WAITING);
            process->setCoreID(-1);
            processQueue.push(process);
            //std::cout << "not ";
        }
        else {
            memoryManager.deallocateMemory(process->getPID());
            std::cout << "Process " << process->getPID() << " completed and deallocated" << std::endl;
        }

        coreAvailable[coreId] = true;   //set to true now since done
        memoryManager.setStatus(process->getPID(), "idle");
        lock.unlock();
        //std::cout << "done for " << process->getPID() << " cpu " << coreId << std::endl;
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

            memoryManager.printMemoryLayout(getActiveTicks() / 5);
            
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
    //std::cout << "inside screenInfo to call getCpuUtilization " << std::endl;
    shortcut << "CPU Utilization: " << getCpuUtilization() << "%" << std::endl;
    shortcut << "Cores used: " << getUsedCores() << std::endl;
    shortcut << "Cores available: " << countAvailCores() << std::endl << std::endl;
    shortcut << "--------------------------------------------------\n";
    shortcut << "Running processes:\n";

    {
        //std::lock_guard<std::mutex> lock(queueMutex);
        for (const auto& process : processes) {
            //std::cout << "checking process " << process->getPID() << " " << process->getCommandCounter() << std::endl;
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
        //std::lock_guard<std::mutex> lock(queueMutex);
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
    //std::lock_guard<std::mutex> lock(queueMutex);
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
    //std::cout << "inside countAvailCores " << std::endl;
    int count = 0;
    if (type == "rr") {
        //std::cout << "rr " << std::endl;
        // count = 0;
        {
            //std::lock_guard<std::mutex> lock(queueMutex);
            //std::cout << "not locking then looping cores " << std::endl;
            for (int coreId = 0; coreId < numCores; ++coreId) {
                //std::cout << "checking core " << coreId << std::endl;
                if (coreAvailable[coreId]) { // Check if core is marked as available
                    count++;
                }
            }
            //std::cout << "final count " << count << std::endl;
        }
    }
    else { // fcfs
        count = numCores;
        {
            //std::lock_guard<std::mutex> lock(queueMutex);
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
    //std::cout << "inside getUsedCores to call countAvailCores " << std::endl;
    int avail = countAvailCores();
    //std::cout << "got the available cores " << avail << std::endl;
    int used = numCores - avail;
    //std::cout << "returning used " << used << std::endl;
    return used;
}

float Scheduler::getCpuUtilization() {
    //std::cout << "inside getCpuUtilization to call getUsedCores " << std::endl;
    auto used = getUsedCores();
    //std::cout << "got the used cores " << used << std::endl;
    float percent = float(used) / numCores * 100;
    //std::cout << "returning percent " << percent << std::endl;
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