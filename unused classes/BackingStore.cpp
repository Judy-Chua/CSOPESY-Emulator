#include "BackingStore.h"

void BackingStore::addProcess(const Process& process) {
    processStore[process.getPID()] = process;
}

Process* BackingStore::getProcess(int pid) {
    if (processStore.find(pid) != processStore.end()) {
        return &processStore[pid];
    }
    return nullptr;
}


