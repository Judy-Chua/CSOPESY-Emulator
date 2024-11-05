#include <memory>
#include <unordered_map>
#include <string>
#include <Windows.h>
#include "AConsole.h"
#include "Process.h"
#include "Scheduler.h"

using namespace std;

class ConsoleManager {
public:
	using String = std::string;
	using Processes = std::vector<std::shared_ptr<Process>>;
	using ConsoleTable = std::unordered_map<String, std::shared_ptr<AConsole>>;

	static ConsoleManager* getInstance();
	static void initialize();
	static void destroy();

	void drawConsole() const;
	void process() const;
	void switchConsole(const std::string& name);
	void returnToPreviousConsole();
	void exitApplication();
	bool isRunning() const;

	HANDLE getConsoleHandle() const;
	void setCursorPosition(int posX, int posY) const;

	void createProcess(const std::string& processName, int lines);
	void setScheduler(Scheduler* scheduler);
	void setConfigInitialize(bool initialize);
	void addCpuCycle();

	int getCpuCycle() const;
	int getCurrentPID() const;
	bool isConfigInitialized() const;

	Processes processes;
	ConsoleTable consoleTable;
	Scheduler* scheduler;

private:
	ConsoleManager();
	~ConsoleManager() = default;
	ConsoleManager(ConsoleManager const&) = delete;
	ConsoleManager& operator=(ConsoleManager const&) = delete;
	static ConsoleManager* instance;

	std::shared_ptr<AConsole> currentConsole;
	std::shared_ptr<AConsole> previousConsole;

	HANDLE consoleHandle;
	bool running = true;
	bool configInitialize = false;
	int cpuCycle = 0;
	int currentPID = 1000;
};

