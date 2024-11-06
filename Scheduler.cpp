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

Scheduler::Scheduler(int numCores, const std::string& type, int timeSlice, int freq, int min, int max, int delay, int memMax, int memFrame, int memProc) :
    numCores(numCores), type(type), coreAvailable(numCores, true), workers(numCores), 
    timeSlice(timeSlice), batchFreq(freq), minIns(min), maxIns(max), delaysPerExec(delay),
    maxOverallMem(memMax), memPerFrame(memFrame), memPerProc(memProc) {}

void Scheduler::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(queueMutex);
    process->setState(Process::READY); //set to READY first
    processes.push_back(process);
    processQueue.push(process);
    //std::cout << "Process added: " << process->getName() << " with PID: " << process->getPID() << "\n";
    cv.notify_one();
}

void Scheduler::startScheduling() {
    std::lock_guard<std::mutex> lock(queueMutex);
    stop = false;
    if (!schedulerThread.joinable()) {
        schedulerThread = std::thread(&Scheduler::schedule, this);
    } 
}

void Scheduler::generateProcesses() {
    if (!generateProcessThread.joinable()) {
        generateProcessThread = std::thread(&Scheduler::generateProcess, this);
    }
}

void Scheduler::stopScheduler() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stop = true;
    }
    cv.notify_all(); 
    if (generateProcessThread.joinable()) {
        generateProcessThread.join();
    }
}

void Scheduler::schedule() {
    type.erase(remove(type.begin(), type.end(), '\"'), type.end());
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
    int lastCycle = -1;
    while (true) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (stop) 
                break;  // Exit if stop is true
        }
        int cpuCycle = ConsoleManager::getInstance()->getCpuCycle();

        if (cpuCycle % batchFreq == 0 && lastCycle != cpuCycle) {
            int random = generateRandomNumber();
            int lastPID = ConsoleManager::getInstance()->getCurrentPID() + 1;
            string initialName = "p";
            string newName = initialName + to_string(lastPID);
            ConsoleManager::getInstance()->createProcess(newName, random);
            lastCycle = cpuCycle;
        }
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}

void Scheduler::scheduleFCFS() {
    MemoryManager memoryManager(maxOverallMem, memPerFrame, memPerProc, maxOverallMem); // the last default value is also max
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
            if (memoryManager.getAvailableMemory() > 0) { // has available memory
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
            else // no more memory
                processQueue.push(process);
        }

        if (!assigned) {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this] {
                return std::any_of(coreAvailable.begin(), coreAvailable.end(), [](bool available) { return available; });
                });
        }
    }

    cv.notify_all(); // Notify all waiting threads in case of a stop
}

void Scheduler::scheduleRR() {
    MemoryManager memoryManager(maxOverallMem, memPerFrame, memPerProc, maxOverallMem); // the last default value is also max
    std::unique_lock<std::mutex> lock(queueMutex);
    while (true) {
        cv.wait(lock, [this] {
            return !processQueue.empty(); // Wait until a process is available in the queue
        });

        std::shared_ptr<Process> process = processQueue.front();
        bool assigned = false;

        // Assign process to an available core
        for (int coreId = 0; coreId < numCores; ++coreId) {
            if (memoryManager.getAvailableMemory() > 0) { // has available memory
                if (coreAvailable[coreId]) {
                    coreAvailable[coreId] = false;
                    assigned = true;

                    processQueue.pop();

                    // Start worker on this core for the process
                    if (workers[coreId].joinable()) {
                        workers[coreId].join();  // Ensure previous worker is finished
                    }
                    workers[coreId] = std::thread(&Scheduler::workerRR, this, coreId, process); // New RR worker
                    break;
                }
            }
            else // no more memory
                processQueue.push(process);
        }
        
        // TODO: idk where to put this
        if (!assigned) {
            cv.wait(lock, [this] {
                return std::any_of(coreAvailable.begin(), coreAvailable.end(), [](bool available) { return available; });
            });
        }
    }

    lock.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// Worker function specific to RR scheduling
void Scheduler::workerRR(int coreId, std::shared_ptr<Process> process) {
    MemoryManager memoryManager(maxOverallMem, memPerFrame, memPerProc, maxOverallMem); // the last default value is also max
    if (process != nullptr && !process->getName().empty()) {
        auto startCycle = ConsoleManager::getInstance()->getCpuCycle();
        int lastCycle = -1;
        process->setCoreID(coreId);
        // Execute process within the time slice for RR
        while (!process->isFinished() && process->getCoreID() == coreId)  {
            auto currentCycle = ConsoleManager::getInstance()->getCpuCycle();
            process->setState(Process::RUNNING);

            if (currentCycle - startCycle >= timeSlice) { // Time slice expired, requeue process
                memoryManager.deallocateMemory(process->getPID());
                process->setState(Process::WAITING);
                process->setCoreID(-1);
                processQueue.push(process);
                coreAvailable[coreId] = true;
                //ConsoleManager::getInstance()->addCpuCycle();

                cv.notify_one(); // Notify scheduler of available core
                return; // Exit worker after requeueing
            }

            if (lastCycle != currentCycle && delaysPerExec == 0) {
                memoryManager.deallocateMemory(process->getPID());
                process->executeCommand(coreId);
                //ConsoleManager::getInstance()->addCpuCycle();
            }

            if (delaysPerExec > 0) {
                memoryManager.allocateMemory(process->getPID());
                process->executeCommand(coreId);
                ConsoleManager::getInstance()->addCpuCycle();
                for (int i = 0; i < delaysPerExec; ++i) {
                    std::this_thread::yield();

                    auto currentCycle = ConsoleManager::getInstance()->getCpuCycle();
                    if (currentCycle - startCycle >= timeSlice) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            }
            lastCycle = currentCycle;
        }
        
        coreAvailable[coreId] = true;
        cv.notify_one(); // Notify scheduler
    }
}

void Scheduler::worker(int coreId, std::shared_ptr<Process> process) {
    MemoryManager memoryManager(maxOverallMem, memPerFrame, memPerProc, maxOverallMem); // the last default value is also max
    if (process != nullptr && !process->getName().empty()) {
        auto currentCycle = ConsoleManager::getInstance()->getCpuCycle();
        auto startCycle = currentCycle;
        process->setStartTime();

        // Process execution
        while (!process->isFinished() && process->getState() != Process::WAITING) {
            memoryManager.allocateMemory(process->getPID());
            process->executeCommand(coreId);
            //ConsoleManager::getInstance()->addCpuCycle();

            if (delaysPerExec > 0) {
                for (int i = 0; i < delaysPerExec; ++i) {
                    std::this_thread::yield();
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            }
            else {
                auto targetCycle = currentCycle + 1; // Prepare for the next cycle
                while (ConsoleManager::getInstance()->getCpuCycle() < targetCycle) {
                    std::this_thread::yield(); // Yield to allow other threads to execute
                }
                currentCycle = ConsoleManager::getInstance()->getCpuCycle(); // Update the current cycle
            }
        }

        memoryManager.deallocateMemory(process->getPID());
        coreAvailable[coreId] = true;

        cv.notify_one();  // Notify scheduler that a core is now available
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
        int runCtr = 0, finCtr = 0, avail = countAvailCores();
        if (type == "rr") {
            avail = countAvailCoresRR();
        }
        int used = numCores - avail;
        float percent = float(numCores - avail) / numCores * 100;

    shortcut << "CPU Utilization: " << percent << "%" << std::endl;
    shortcut << "Cores used: " << used << std::endl;
    shortcut << "Cores available: " << avail << std::endl << std::endl;
    shortcut << "--------------------------------------------------\n";
    shortcut << "Running processes:\n";

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (const auto& process : processes) {
            string coreID;
            if (process->getCoreID() != -1) { coreID = to_string(process->getCoreID()); }
            else { coreID = "N/A"; }

            if (process->getState() != Process::FINISHED && process->getCoreID() != -1) {
                shortcut << process->getName() << "\tStarted: " << process->getStartTime()
                    << "   Core: " << coreID << "   " << process->getCommandCounter() << " / "
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

int Scheduler::countAvailCores() {
    int count = numCores;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (const auto& process : processes) { //more updated version instead of checking coresAvailable
            if (process->getState() == Process::RUNNING && process->getCoreID() != -1) {
                count--;
            }
        }
    }
    return count;
}

int Scheduler::countAvailCoresRR() {
    int count = 0;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (int coreId = 0; coreId < numCores; ++coreId) {
            if (coreAvailable[coreId]) { // Check if core is marked as available
                count++;
            }
        }
    }
    return count;
}

int Scheduler::generateRandomNumber() {
    random_device random;
    mt19937 generate(random());
    uniform_int_distribution<> distr(minIns, maxIns);
    return distr(generate);
}