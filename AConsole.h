#pragma once
#include <string>

class AConsole {
public:
    AConsole(const std::string& name);
    virtual ~AConsole() = default;

    std::string getName() const;

    virtual void onEnabled() = 0;
    virtual void display() = 0;
    virtual void process() = 0;
    virtual bool isDone() = 0;

protected:
    std::string name;
};
