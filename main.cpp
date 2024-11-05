#include "ConsoleManager.h"
#include "MainConsole.h"

int main() {
    ConsoleManager::initialize();
    ConsoleManager* consoleManager = ConsoleManager::getInstance();

    auto mainConsole = std::make_shared<MainConsole>();
    consoleManager->consoleTable["MAIN_CONSOLE"] = mainConsole;
    consoleManager->switchConsole("MAIN_CONSOLE");

    while (consoleManager->isRunning()) {
        consoleManager->process();

        if (consoleManager->isConfigInitialized()) {
            consoleManager->addCpuCycle();
        }
    }

    ConsoleManager::destroy();
    return 0;
}