#include "AConsole.h"

AConsole::AConsole(const std::string& name) : name(name) {}

std::string AConsole::getName() const {
    return name;
}
