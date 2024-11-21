#include "BackingStore.h"
#include <iostream>
#include <fstream>

void BackingStore::addProcess(std::shared_ptr<Process> process, int pid) {
    processStore[pid] = process;
}

std::shared_ptr<Process> BackingStore::getProcess(int pid) {
    if (processStore.find(pid) != processStore.end()) {
        return processStore[pid];
    }
    return nullptr;
}

void BackingStore::storeProcess(const string& name, int pid, int commandLine) {
    std::ofstream outFile;

    outFile.open("backing-store.txt");

    if (outFile.is_open()) {
        outFile << name << " " << pid << commandLine << "\n";
        outFile.close();
    }
}


