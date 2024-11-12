#include "BaseScreen.h"
#include "Process.h"
#include "ConsoleManager.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <string>
#include <sstream>
using String = std::string;

BaseScreen::BaseScreen(const std::string& processName, std::shared_ptr<Process> process) :
            AConsole(processName), thisProcess(process) {
    time_t now = time(nullptr);
    tm localTime;
    localtime_s(&localTime, &now);

    std::ostringstream timeString;
    timeString << std::put_time(&localTime, "%m/%d/%Y, %I:%M:%S %p"); 
                                        //(MM/DD/YYYY, HH:MM:SS AM/PM) format
    timeCreated = timeString.str();
}

void BaseScreen::onEnabled() {
    refreshed = false;
    display();
}

void BaseScreen::display() {
    if (!refreshed) {
        system("cls");
        printProcessInfo();
        refreshed = true;
    }
    process();
}

void BaseScreen::process() {
    String input;

    std::cout << "\033[94mroot:\\>";
    std::cout << "\033[97m ";
    std::getline(std::cin, input);  // Use getline to capture the full input

    if (input == "exit") {
        ConsoleManager::getInstance()->returnToPreviousConsole();
    }
    else if (input == "process-smi") {
        updates(); //double check
    }
    else {
        std::cout << "Invalid command, please try again. Type 'exit' to go back.\n" << std::endl;
    }
}

void BaseScreen::printProcessInfo() {
    std::cout << "\033[97m";
    std::cout << "Process Name: " << name << std::endl;
    std::cout << "ID: " << thisProcess->getPID() << std::endl;
    std::cout << "Created: " << timeCreated << std::endl;
    std::cout << "Current Line of Instruction: "
        << thisProcess->getCommandCounter() << " / " << thisProcess->getLinesOfCode() << std::endl;
    if (isDone())
        std::cout << "Process '" << name << "' has finished executing!" << std::endl;
    std::cout << std::endl;
}

void BaseScreen::updates() {
    std::cout << "\033[97m";
    std::cout << "Process Name: " << name << std::endl;
    std::cout << "ID: " << thisProcess->getPID() << std::endl << std::endl;
  
    if (isDone()) {
        std::cout << "Finished!" << std::endl;
    }
    else {
        std::cout << "Current Line of Instruction: " << thisProcess->getCommandCounter() << std::endl;
        std::cout << "Lines of code: " << thisProcess->getLinesOfCode() << std::endl;
    }
    std::cout << std::endl;
}

bool BaseScreen::isDone() {
    if (thisProcess->getCommandCounter() == thisProcess->getLinesOfCode())
        return true;
    return false;
}