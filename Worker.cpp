#include "Worker.h" 

Worker::Worker(int coreId, const std::string& type) : coreId(coreId), type(type), 
    activeTicks(0), idleTicks(0), delay(0), timeSlice(-1) {}

void Worker::assignWork(std::shared_ptr<Process> process, int delay, int timeSlice) {
    this->process = process;
    this->delay = delay;
    this->timeSlice = timeSlice;
    assigned = true;
    
    if (type == "fcfs") {
        executeFCFS();
    }
    else {
        executeRR();
    }
}

void Worker::executeFCFS() {
    if (process != nullptr && !process->getName().empty()) {
        process->setState(Process::RUNNING);
        process->setCoreID(coreId);

        while (!process->isFinished()) {// Execute process
            process->executeCommand(coreId);
            activeTicks++;
            std::this_thread::sleep_for(chrono::milliseconds(delay));
        }

        assigned = false;   //set to false now since done

        if (!process->isFinished()) {   //process not yet done
            process->setState(Process::WAITING);
            process->setCoreID(-1);
        }
    }
}

void Worker::executeRR() {
    if (process != nullptr && !process->getName().empty()) {
        process->setState(Process::RUNNING);
        process->setCoreID(coreId);
        int ctr = 0;

        // Execute process within the time slice for RR
        while (ctr < timeSlice && !process->isFinished()) {
            process->executeCommand(coreId);
            ctr++;
            activeTicks++;
            std::this_thread::sleep_for(chrono::milliseconds(delay));
        }

        assigned = false;   //set to false now since done

        if (!process->isFinished()) {   //process not yet done
            process->setState(Process::WAITING);
            process->setCoreID(-1);
        }
    }
}

void Worker::startCountTicks() {
    if (!assigned) {
        idleTicks++;
    }
}
int Worker::getActiveTicks() const {
    return activeTicks;
}
int Worker::getIdleTicks() const {
    return idleTicks;
}

bool Worker::isAssigned() const {
    return assigned;
}
