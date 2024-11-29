#include "MainConsole.h"
#include "ConsoleManager.h"
#include "BaseScreen.h"
#include "Scheduler.h"
#include "Process.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>


MainConsole::MainConsole() : AConsole("MAIN_CONSOLE"), x(1) {}

void MainConsole::onEnabled() {
    display();
}

void MainConsole::display() {
    std::cout << "\033[97m";
    std::cout << "  _____ _____   ___  _____  ______ _______   __  " << "\n";
    std::cout << " /  ___/  ___| / _ \\|  _  \\|  ____/  ___\\ \\ / / " << "\n";
    std::cout << "|  |   \\____ \\| | | | |_>  |  __| \\____ \\\\ V / " << "\n";
    std::cout << "|  |___ ____> | |_| |  ___/|  |___ ____> || | " << "\n";
    std::cout << " \\_____|_____/ \\___/|_|    |______|_____/ |_| " << "\n";
    std::cout << "------------------------------------------------\n";
    std::cout << "\033[92mWelcome to CSOPESY Emulator!\n\n";
    std::cout << "\033[95mDevelopers:\n";
    std::cout << "\033[94m   CHUA, Judy\n";
    std::cout << "   TELOSA, Arwyn Gabrielle\n";
    std::cout << "   UY, Jasmine Louise\n";
    std::cout << "   VALENZUELA, Shanley\n\n";
    std::cout << "\033[95mLast updated: " << "\033[93m11-29-2024\n";
    std::cout << "\033[97m------------------------------------------------\n\n";
}

void MainConsole::process() {
    std::cout << "\033[91mroot:\\>";
    std::cout << "\033[97m ";
    std::getline(std::cin, userInput);

    Command command = getCommand(userInput);
    std::cout << "\033[96m";
    executeCommand(command, userInput);
}

MainConsole::Command MainConsole::getCommand(const std::string& input) const {
    std::istringstream iss(input);
    std::string word;
    iss >> word;
    word = toLowerCase(word);

    if (word == "initialize") return CMD_INITIALIZE;
    else if (word == "screen") return isInitialized ? CMD_SCREEN : CMD_NOT_INITIALIZED;
    else if (word == "scheduler-test") return isInitialized ? CMD_SCHEDULER_TEST : CMD_NOT_INITIALIZED;
    else if (word == "scheduler-stop") return isInitialized ? CMD_SCHEDULER_STOP : CMD_NOT_INITIALIZED;
    else if (word == "report-util") return isInitialized ? CMD_REPORT_UTIL : CMD_NOT_INITIALIZED;
    else if (word == "process-smi") return isInitialized ? CMD_PROCESS_SMI : CMD_NOT_INITIALIZED;
    else if (word == "vmstat") return isInitialized ? CMD_VMSTAT : CMD_NOT_INITIALIZED;
    else if (word == "clear") return CMD_CLEAR;
    else if (word == "exit") return CMD_EXIT;
    else return CMD_INVALID;
}

struct Config {
    int numCpu = 4;
    std::string scheduler = "rr";
    int quantumCycles = 5;
    int batchProcessFreq = 1;
    int minIns = 1000;
    int maxIns = 2000;
    int delayPerExec = 0;
    int maxOverallMem = 2;
    int memPerFrame = 2;
    int minMemPerProc = 2;
    int maxMemPerProc = 2;
};

Config readConfig(const std::string& filename) {
    Config config;
    std::ifstream file(filename);

    if (!file) {
        std::cerr << "Config file not found. Using default values.\n";
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        int value;

        if (line.find("num-cpu") != std::string::npos) {
            iss >> key >> value;
            config.numCpu = value;  
        } else if (line.find("scheduler") != std::string::npos) {
            iss >> key >> config.scheduler;
            config.scheduler = config.scheduler.substr(1, config.scheduler.length() - 2);
        } else if (line.find("quantum-cycles") != std::string::npos) {
            iss >> key >> value;
            config.quantumCycles = value;  
        }  else if (line.find("batch-process-freq") != std::string::npos) {
            iss >> key >> value;
            config.batchProcessFreq = value;  
        } else if (line.find("min-ins") != std::string::npos) {
            iss >> key >> value;
            config.minIns = value;  
        } else if (line.find("max-ins") != std::string::npos) {
            iss >> key >> value;
            config.maxIns = value;  
        } else if (line.find("delay-per-exec") != std::string::npos) {
            iss >> key >> value;
            config.delayPerExec = value;  
        } else if (line.find("max-overall-mem") != std::string::npos) {
            iss >> key >> value;
            config.maxOverallMem = value;
        } else if (line.find("mem-per-frame") != std::string::npos) {
            iss >> key >> value;
            config.memPerFrame = value;
        } else if (line.find("min-mem-per-proc") != std::string::npos) {
            iss >> key >> value;
            config.minMemPerProc = value;
        } else if (line.find("max-mem-per-proc") != std::string::npos) {
            iss >> key >> value;
            config.maxMemPerProc = value;
        }
    }

    return config;
}

void MainConsole::executeCommand(Command command, const std::string& userInput){
    switch (command) {
    case CMD_INITIALIZE: {
        Config config = readConfig("config.txt");

        if (scheduler == nullptr) {
            scheduler = new Scheduler(config.numCpu, config.scheduler, config.quantumCycles,
                config.batchProcessFreq, config.minIns, config.maxIns, config.delayPerExec,
                config.maxOverallMem, config.memPerFrame, config.minMemPerProc, config.maxMemPerProc);
            scheduler->startScheduling();
            ConsoleManager::getInstance()->setScheduler(scheduler);
            isInitialized = true;
            std::string sched;
            if (config.scheduler == "rr") {
                sched = "Round Robin";
            }
            else sched = "First Come First Serve";

            std::cout << "Program configuration initialized!\n";
            std::cout << "CPU settings set to:" << std::endl;
            std::cout << "   Number of CPUs                - " << config.numCpu << std::endl;
            std::cout << "   Scheduler                     - " << sched << std::endl;
            std::cout << "   Quantum Cycles                - " << config.quantumCycles << std::endl;
            std::cout << "   Frequency of Adding Processes - " << config.batchProcessFreq << std::endl;
            std::cout << "   Range of Instructions         - " << config.minIns << "-" << config.maxIns << std::endl;
            std::cout << "   Delay per Execution           - " << config.delayPerExec << std::endl << std::endl;

            std::cout << "Memory settings set to:" << std::endl;
            std::cout << "   Maximum Memory Available      - " << config.maxOverallMem << std::endl;
            std::cout << "   Memory Size per Frame         - " << config.memPerFrame << std::endl;
            std::cout << "   Minimum Size per Process      - " << config.minMemPerProc << std::endl;
            std::cout << "   Maximum Size Per Process      - " << config.maxMemPerProc << std::endl << std::endl;

            x = config.batchProcessFreq;
        }
        else {
            std::cout << "Scheduler already initialized.\n" << std::endl;
        }
        break;
    }
    case CMD_NOT_INITIALIZED: {
        std::cout << "ERROR: Initialize the configuration before using other commands.\n" << std::endl;
        break;
    }
    case CMD_SCREEN: {
        if (!isInitialized) {
            std::cout << "ERROR: Initialize the configuration first.\n";
            break;
        }
        std::istringstream iss(userInput);
        std::string word, mode, parameter;

        iss >> word;      // Command 'screen'
        iss >> mode;      // Mode '-r' or '-s'
        iss >> parameter; // Process name

        if (mode == "-r") {
            if (ConsoleManager::getInstance()->consoleTable.find(parameter) != ConsoleManager::getInstance()->consoleTable.end()) {
                ConsoleManager::getInstance()->switchConsole(parameter);
            }
            else {
                std::cout << "No screen found with the name: " << parameter << "\n" << std::endl;
            }
        }
        else if (mode == "-s") {
            auto& consoleTable = ConsoleManager::getInstance()->consoleTable;
            auto screenIt = consoleTable.find(parameter);
            if (screenIt != consoleTable.end()) {
                // If the screen exists and is done, remove it and create a new one
                if (screenIt->second->isDone()) {
                    consoleTable.erase(screenIt);
                }
                else {
                    std::cout << "ERROR: Screen " << parameter << " already exists and is not finished!\n" << std::endl;
                    break;
                }
            }

            std::cout << "Creating new screen: " << parameter << "\n" << std::endl;

            int random = scheduler->generateInstructions();
            int memory = scheduler->generateMemory();

            ConsoleManager::getInstance()->createProcess(parameter, random, memory);
            ConsoleManager::getInstance()->switchConsole(parameter);
        }
        else if (mode == "-ls") {
            if (scheduler != nullptr)
                scheduler->printActiveScreen();
            break;
        }
        else {
            std::cout << "Invalid screen option. Use '-r', '-s', or '-ls'.\n" << std::endl;
        }
        break;
    }
    case CMD_SCHEDULER_TEST: {
        std::cout << "Scheduler is now generating dummy processes every "
            << x << " CPU cycle/s...\n\n";

        if (schedulerThread.joinable()) {
            std::cout << "Scheduler thread is already running.\n\n";
            break;
        }

        schedulerThread = std::thread([this]() {
            try {
                this->scheduler->generateProcesses();
                this->scheduler->startTicksProcesses();
            }
            catch (const std::exception& e) {
                std::cerr << "Scheduler error: " << e.what() << std::endl;
            }
            });
        break;
    }
    case CMD_SCHEDULER_STOP: {
        std::cout << "Scheduler has stopped generating dummy processes...\n";
        if (scheduler != nullptr) {
            scheduler->stopScheduler();
            if (schedulerThread.joinable()) {
                schedulerThread.join();
            }
        }
        else {
            std::cout << "No scheduler running.\n";
        }
        std::cout << std::endl;
        break;
    }
    case CMD_REPORT_UTIL: {
        std::cout << "Generating utilization report...\n" << std::endl;
        scheduler->reportUtil();
        break;
    }
    case CMD_PROCESS_SMI: {
        if (scheduler != nullptr)
            scheduler->printProcessSMI();
        break;
    }
    case CMD_VMSTAT: {
        if (scheduler != nullptr)
            scheduler->printVmstat();
        break;
    }
    case CMD_CLEAR: {
        system("cls");  // Clear the screen
        display();
        break;
    }
    case CMD_EXIT: {
        std::cout << "\033[31mExiting program...\n" << std::endl;
        std::cout << "\033[97m";
        if (schedulerThread.joinable()) {
            schedulerThread.join();
        }
        ConsoleManager::getInstance()->exitApplication();
        break;
    }
    case CMD_INVALID:
    default:
        std::cout << "ERROR: Invalid command. Please try again.\n" << std::endl;
        break;
    }
}


std::string MainConsole::toLowerCase(const std::string& str) const {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

bool MainConsole::isDone() {
    return false; //no use
}