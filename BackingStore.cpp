#include "BackingStore.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

BackingStore::BackingStore() {}

void BackingStore::addProcess(std::shared_ptr<Process> process, int pid) {
    processStore[pid] = process;
}

std::shared_ptr<Process> BackingStore::getProcess(int pid) {
    if (processStore.find(pid) != processStore.end()) {
        return processStore[pid];
    }
    return nullptr;
}

void BackingStore::removeProcess(int pid) {
	if (processStore.find(pid) != processStore.end()) {
		processStore.erase(pid);
	}
}

void BackingStore::storeProcess(int pid) {
    std::ofstream outFile;

    outFile.open("backing-store.txt", std::ios::app);

    if (outFile.is_open()) {
        std::cout << "storing is " << pid << std::endl;
        auto now = std::chrono::system_clock::now();
        time_t currentTime = std::chrono::system_clock::to_time_t(now);
        struct tm buf;
        localtime_s(&buf, &currentTime);
        char timeStr[100];
        strftime(timeStr, sizeof(timeStr), "%m/%d/%Y %I:%M:%S %p", &buf);

        outFile << processStore[pid]->getName() << " " << pid << " "
            << processStore[pid]->getCommandCounter() << " / "
            << processStore[pid]->getLinesOfCode() << " ("
            << timeStr << ")\n";

        outFile.close();
    }
}


