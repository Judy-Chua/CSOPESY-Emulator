#include "worker.h"
#include "process.h"
#include <iostream>
#include <string>
#include <random>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <atomic>

std::mutex processQueueMutex;
std::condition_variable processQueueCV;
std::atomic<bool> ready(false);
std::queue<ProcessInfo> processQueue;
std::vector<ProcessInfo> processList;
std::vector<std::unique_ptr<Worker>> workers;


static void initializeWorker() {
    for (int i = 0; i < 4; ++i) {
        workers.push_back(std::make_unique<Worker>(i, 250, 5));
    }
}

static int generateRandomNumber() {
    static std::random_device random;
    static std::mt19937 generate(random());
    static std::uniform_int_distribution<> distr(1, 30);

    return distr(generate);
}

static void generateProcess() {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(processQueueMutex);
            processQueueCV.wait(lock, [] { return ready.load(); });
        }

        std::cout << "Generating process..." << std::endl;

        int i = 0;
        while (ready.load()) {
            int number = generateRandomNumber();
            ProcessInfo newProcess = { 0, number, 1000 + i, 0, -1 };
            {
                std::lock_guard<std::mutex> lock(processQueueMutex);
                processQueue.push(newProcess);
                processList.push_back(newProcess);
            }

            i++;
            processQueueCV.notify_one();
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
}

static void schedule() {
    while (true) {
        ProcessInfo process;

        {
            std::unique_lock<std::mutex> lock(processQueueMutex);
            processQueueCV.wait(lock, [] { return !processQueue.empty(); });
            if (!ready.load() && processQueue.empty()) {
                return; // Exit or handle graceful shutdown
            }

            process = std::move(processQueue.front());
            processQueue.pop();
        }

        bool assigned = false;

        // Try to assign the process to an available worker
        for (auto& workerPtr : workers) {
            Worker& worker = *workerPtr; // Dereference the unique_ptr
            if (!worker.isAssigned()) {
                worker.assignWork(process);
                worker.executeRR();
                assigned = true;
                break;
            }
        }

        // If no worker is available, requeue the process
        if (!assigned) {
            {
                std::lock_guard<std::mutex> lock(processQueueMutex);
                processQueue.push(std::move(process));
            }
            processQueueCV.notify_one(); // Notify scheduler of updated queue
        }
    }
}

int main() {
    initializeWorker();
    std::string input;
    std::thread processThread(generateProcess);

    while (true) {
        std::cout << "Enter command: ";
        std::getline(std::cin, input);

        if (input == "p") {
            {
                std::lock_guard<std::mutex> lock(processQueueMutex);
                ready = true;
            }
            processQueueCV.notify_one();
        }
        else if (input == "stat") {
            std::lock_guard<std::mutex> lock(processQueueMutex);
            std::cout << "All current processes.\n";
            for (const auto& process : processList) {
                std::cout << process.pid << " "
                    << process.count << " / "
                    << process.total << "   "
                    << "Core ID: " << (process.coreId == -1 ? "Unassigned" : std::to_string(process.coreId)) << "   "
                    << "State: " << process.state << "\n";
            }
            std::cout << std::endl;
        }
        else if (input == "work") {
            schedule();
        }
        else if (input == "stop") {
            {
                std::lock_guard<std::mutex> lock(processQueueMutex);
                ready = false;
            }
        }
        else if (input == "exit") {
            {
                std::lock_guard<std::mutex> lock(processQueueMutex);
                ready = false;
            }
            
            for (auto& workerPtr : workers) {
                workerPtr->stop();
            }
            processThread.join();

            std::cout << "Exiting...\n";
            break;
        }
    }

    return 0;
}