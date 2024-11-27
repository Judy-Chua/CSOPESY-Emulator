#pragma once
#include "AConsole.h"
#include "Process.h"
#include <memory>
#include <string>

class BaseScreen : public AConsole {
public:
    BaseScreen(const std::string& processName, std::shared_ptr<Process> process);
    void onEnabled() override;
    void display() override;
    void process() override;
    bool isDone() override;

    void updates();

private:
    void printProcessInfo();
    bool refreshed = false;
    std::string timeCreated;
    std::shared_ptr<Process> thisProcess;
};

