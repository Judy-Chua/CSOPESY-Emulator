#pragma once
#include "Process.h"
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

class BackingStore {

public:
    BackingStore();
    void addProcess(std::shared_ptr<Process> process, int pid);
    std::shared_ptr<Process> getProcess(int pid);
	void removeProcess(int pid);
    void storeProcess(int pid);

private:
    std::unordered_map<int, std::shared_ptr<Process>> processStore;
};
