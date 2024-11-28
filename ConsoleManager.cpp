#include "ConsoleManager.h"
#include "BaseScreen.h"
#include "MainConsole.h"
#include "Process.h"
#include <iostream>

// Initialize the static singleton instance to nullptr
ConsoleManager* ConsoleManager::instance = nullptr;

// Constructor
ConsoleManager::ConsoleManager() {
    this->running = true;
    this->consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    // Initialize the main console screen
    auto mainConsole = std::make_shared<MainConsole>();
    this->consoleTable["MAIN_CONSOLE"] = mainConsole;

    this->switchConsole("MAIN_CONSOLE");
}

// Get singleton instance
ConsoleManager* ConsoleManager::getInstance() {
    if (!instance) {
        instance = new ConsoleManager();
    }
    return instance;
}

// Initialize the singleton instance
void ConsoleManager::initialize() {
    if (!instance) {
        instance = new ConsoleManager();
    }
}

// Destroy the singleton instance
void ConsoleManager::destroy() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

// Draw the current console
void ConsoleManager::drawConsole() const {
    if (currentConsole) {
        currentConsole->display();
    }
}

// Process the current console logic
void ConsoleManager::process() const {
    if (currentConsole) {
        currentConsole->process();
    }
}

// Switch to another console by name
void ConsoleManager::switchConsole(const std::string& name) {
    auto processScreen = consoleTable.find(name);
    if (processScreen != consoleTable.end()) {
        if (processScreen->second->isDone()) {
            std::cout << "Can't access screen '" << name << "'. (Already done executing)\n" << std::endl;
        }
        else {
            system("cls");
            previousConsole = currentConsole;
            currentConsole = consoleTable[name];
            currentConsole->onEnabled();
        }
    }
    else {
        std::cout << "Console not found: " << name << std::endl;
    }
}

// Return to the previous console
void ConsoleManager::returnToPreviousConsole() {
    if (previousConsole) {
        system("cls");
        currentConsole = previousConsole;
        previousConsole.reset();
        currentConsole->onEnabled();  // Assuming BaseScreen has an onEnabled method
    }
}

// Exit the application by setting running to false
void ConsoleManager::exitApplication() {
    running = false;
}

// Check if the application is still running
bool ConsoleManager::isRunning() const {
    return running;
}

// Get the console handle for system operations
HANDLE ConsoleManager::getConsoleHandle() const {
    return consoleHandle;
}

// Set the cursor position in the console
void ConsoleManager::setCursorPosition(int posX, int posY) const {
    COORD position = { static_cast<SHORT>(posX), static_cast<SHORT>(posY) };
    SetConsoleCursorPosition(consoleHandle, position);
}

// Creates processes and its respective screen then adds that process to the scheduler
void ConsoleManager::createProcess(const std::string& processName, int lines, int memory) {
    int newPID = processes.empty() ? 1001 : processes.back()->getPID() + 1;
    currentPID = newPID;
   
    // get start time
    auto now = chrono::system_clock::now();
    time_t currentTime = chrono::system_clock::to_time_t(now);
    struct tm buf;
    localtime_s(&buf, &currentTime);

    char timeStr[100]; // timestamp in timeStr
    strftime(timeStr, sizeof(timeStr), "%m/%d/%Y %I:%M:%S %p", &buf);

    //std::cout << "in console manager, creating new process: " << processName << std::endl;

    auto newProcess = std::make_shared<Process>(newPID, processName, lines, timeStr, memory);
    processes.push_back(newProcess);
    auto processScreen = std::make_shared<BaseScreen>(processName, newProcess);
    consoleTable[processName] = processScreen;

    //std::cout << "in console manager, adding back: " << processName << std::endl;
    scheduler->addProcess(newProcess);
}

// Sets the scheduler based on initialization in Main Console
void ConsoleManager::setScheduler(Scheduler* scheduler) {
    this->scheduler = scheduler;
}

int ConsoleManager::getCurrentPID() const {
    //std::cout << "in console manager, get current pid: " << currentPID << std::endl;
    return currentPID;
}