#pragma once
#include "Process.h"
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

class BackingStore {

public:
    void addProcess(const Process& process);
    Process* getProcess(int pid);

private:
    std::unordered_map<int, Process> processStore;
};
