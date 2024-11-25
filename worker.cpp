#include "worker.h"
#include <iostream>

Worker::Worker(int coreId, int delay, int timeSlice)
    : coreId(coreId), activeTicks(0), idleTicks(0), currentProcess(),
      assigned(false), running(true), delay(delay), timeSlice(timeSlice) {
    tickThread = std::thread(&Worker::startCountTicks, this); //call tickCounter
}

Worker::Worker(Worker&& other) noexcept
    : coreId(other.coreId),
    delay(other.delay),
    timeSlice(other.timeSlice),
    activeTicks(other.activeTicks),
    idleTicks(other.idleTicks),
    assigned(other.assigned),
    running(other.running),
    currentProcess(std::move(other.currentProcess)) {
    if (other.workerThread.joinable())
        workerThread = std::move(other.workerThread);
    if (other.tickThread.joinable())
        tickThread = std::move(other.tickThread);
}

// Move assignment operator
Worker& Worker::operator=(Worker&& other) noexcept {
    if (this != &other) {
        stop();
        std::lock_guard<std::mutex> lock(stateMutex);

        coreId = other.coreId;
        delay = other.delay;
        timeSlice = other.timeSlice;
        activeTicks = other.activeTicks;
        idleTicks = other.idleTicks;
        assigned = other.assigned;
        running = other.running;
        currentProcess = std::move(other.currentProcess);

        if (other.workerThread.joinable())
            workerThread = std::move(other.workerThread);
        if (other.tickThread.joinable())
            tickThread = std::move(other.tickThread);
    }
    return *this;
}

Worker::~Worker() {
	stop();
    if (tickThread.joinable()) {
		tickThread.join();
    }
}

void Worker::assignWork(const ProcessInfo& process) {
    std::lock_guard<std::mutex> lock(mtx);
    this->currentProcess = process;
    assigned = true;
}

void Worker::executeRR() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (!assigned) return;
    }
    currentProcess.state = 1;
    currentProcess.coreId = coreId;
    workerThread = std::thread([this]() {
        int ctr = 0;

        while (ctr < timeSlice) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (!running) break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            ctr++;

            {
                std::lock_guard<std::mutex> lock(mtx);
                activeTicks++;
                currentProcess.count += 1;

                if (currentProcess.count == currentProcess.total) {
                    currentProcess.state = 3;
                    break;
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            if (currentProcess.count < currentProcess.total) {
                currentProcess.state = 2;
                currentProcess.coreId = -1;
            }
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            assigned = false; 
        }
    });
}

void Worker::startCountTicks() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (!running) break; // Exit if the worker is stopped
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(delay));

        {
            std::lock_guard<std::mutex> lock(mtx);
            if (!isAssigned()) idleTicks++;
        }
    }
}

void Worker::stop() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        running = false;
    }
    if (workerThread.joinable())
        workerThread.join();
    if (tickThread.joinable())
		tickThread.join();
}

int Worker::getActiveTicks() const {
    std::lock_guard<std::mutex> lock(mtx);
    return activeTicks;
}

int Worker::getIdleTicks() const {
    std::lock_guard<std::mutex> lock(mtx);
    return idleTicks;
}

bool Worker::isAssigned() const {
    std::lock_guard<std::mutex> lock(mtx);
    return assigned;
}
