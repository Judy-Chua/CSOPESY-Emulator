#pragma once
#include "Process.h"
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

class BackingStore {

public:
    void addProcess(std::shared_ptr<Process> process, int pid);
    void storeProcess(const string& name, int pid, int commandLine);
    std::shared_ptr<Process> getProcess(int pid);

private:
    std::unordered_map<int, std::shared_ptr<Process>> processStore;
};
