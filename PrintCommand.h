#pragma once
#include <string>
#include <vector>

class PrintCommand {
public:
	PrintCommand(const std::string& str);
	void execute(int coreID);

private:
	std::string name;
	std::vector<std::string> output;
};