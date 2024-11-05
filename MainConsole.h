#pragma once
#include "AConsole.h"
#include "Scheduler.h"

class MainConsole : public AConsole {
public:
    MainConsole();
    void onEnabled() override;
    void display() override;
    void process() override;
    bool isDone() override;

    bool isInitialized = false;

private:
    Scheduler* scheduler = nullptr; 
    std::thread schedulerThread;
    std::thread printThread;
    std::mutex screenMutex;
    enum Command {
        CMD_INITIALIZE,
        CMD_NOT_INITIALIZE,
        CMD_SCREEN,
        CMD_SCREEN_ACTIVE,
        CMD_SCHEDULER_TEST,
        CMD_SCHEDULER_STOP,
        CMD_REPORT_UTIL,
        CMD_CLEAR,
        CMD_EXIT,
        CMD_INVALID,
        special
    };
    std::string userInput;

    Command getCommand(const std::string& input) const;
    void executeCommand(Command command, const std::string& userInput);
    std::string toLowerCase(const std::string& str) const;

    int x;
};


