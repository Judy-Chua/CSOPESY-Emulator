Chua, Judy
Telosa, Arwyn
Uy, Jasmine
Valenzuela, Shanley

How to run the OS Emulator:
1. In Visual Studio 2022, create a project and add all the program files
2. Have a file "config.txt" that contains the configurations in the following format:
num-cpu 4
scheduler "rr"
quantum-cycles 5
batch-process-freq 1
min-ins 1000
max-ins 2000
delay-per-exec 0
3. Build and run the project in Visual Studio 2022
4. Enter "initialize" command. The scheduler will automatically start using the given config.
5. Create processes using the "screen -s <process name>" command or the "scheduler-test" command.
6. View running processes using "screen -ls" command
7. Generate a report of all the processes using "report-util command"
8. Use "stop-scheduler" to stop the scheduler.
9. Enter "exit" to exit the program. It will not exit properly if the scheduler is still running.