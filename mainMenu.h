#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <iostream>
#include <string>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <set>
#include "CommandHandler.h"
#include "Config.h"
#include "Scheduler.h"
#include "Memory.h"

class MainMenu {
private:
    CommandHandler cmdHandler;
    bool clearOnCommand;
    Scheduler* scheduler;
    SystemConfig config;
    MemoryManager* memoryManager;   // NEW: global memory manager

    void displayBanner(){
        std::cout << "  _____   _____   ____   _____  ______  _____  __     __" << std::endl;
        std::cout << " / ____| / ____| / __ \\ |  __ \\|  ____|/ ____| \\ \\   / /" << std::endl;
        std::cout << "| |     | (___  | |  | || |__) | |__  | (___    \\ \\_/ / " << std::endl;
        std::cout << "| |      \\___ \\ | |  | ||  ___/|  __|  \\___ \\    \\   /  " << std::endl;
        std::cout << "| |____  ____) || |__| || |    | |____ ____) |    | |   " << std::endl;
        std::cout << " \\_____| |_____/ \\____/ |_|    |______|_____/     |_|   " << std::endl;
    }

    void clearScreen() {
        #ifdef _WIN32
            std::system("cls");
        #else
            std::system("clear");
        #endif
    }

    std::string getUserInput() {
        std::string input;
        std::cout << "Enter command:  ";
        std::getline(std::cin, input);
        return input;
    }

    bool requiresInitialization(const std::string& cmd) {
        return (cmd == "screen-ls" || 
                cmd == "scheduler-start" || 
                cmd == "scheduler-stop" ||
                cmd == "report-util" ||
                cmd == "process-smi");
    }

public:
    MainMenu() 
        : clearOnCommand(false), 
          scheduler(nullptr),
          memoryManager(nullptr) {}   // NEW

    ~MainMenu() {
        if (scheduler) {
            scheduler->stop();
            delete scheduler;
        }
        if (memoryManager) {          // NEW
            delete memoryManager;
        }
    }

    // Setup all commands
    void setupCommands() {
        // Initialize command
        cmdHandler.registerCommand("initialize", [this]() {
            handleInitialize();
        });

        // Screen list command
        cmdHandler.registerCommand("screen-ls", [this]() {
            handleScreenList();
        });

        // Start scheduler command
        cmdHandler.registerCommand("scheduler-start", [this]() {
            handleSchedulerStart();
        });

        // Stop scheduler command
        cmdHandler.registerCommand("scheduler-stop", [this]() {
            handleSchedulerStop();
        });

        // Report utilities command
        cmdHandler.registerCommand("report-util", [this]() {
            handleReportUtil();
        });

        // Clear screen command
        cmdHandler.registerCommand("clear", [this]() {
            clearScreen();
            displayBanner();
        });

        // Help command
        cmdHandler.registerCommand("help", [this]() {
            cmdHandler.showHelp();
        });

        // Exit command
        cmdHandler.registerCommand("exit", [this]() {
            handleExit();
        });

        cmdHandler.registerCommand("vmstat", [this]() {
            if (memoryManager) {
                memoryManager->displayVMStat();
            } else {
                std::cout << "Memory manager not initialized.\n";
            }
        });

        cmdHandler.registerCommand("process-smi", [this]() {
            if (scheduler && memoryManager) {
                displayProcessSMI("");  // Empty string = show all processes
            } else {
                std::cout << "ERROR: System not initialized.\n";
            }
        });
    }

    // Main menu loop
    void run() {
        clearScreen();
        displayBanner();
        
        std::cout << "Type 'help' to see available commands.\n";
        std::cout << "Type 'initialize' to set up the system.\n\n";

        while (cmdHandler.shouldContinue()) {
            if (scheduler && memoryManager) {
                scheduler->deallocateFinishedProcesses(memoryManager);
            }
            std::string userInput = getUserInput();

            // Skip empty input
            if (userInput.empty()) {
                continue;
            }

            // Check if initialization is required
            if (!cmdHandler.isSystemReady() && requiresInitialization(userInput)) {
                std::cout << "ERROR: System not initialized. Please run 'initialize' first.\n\n";
                continue;
            }

            // Try to execute the command
            if (!cmdHandler.executeCommand(userInput)) {
                // Handle special commands with parameters (like screen -r, screen -s)
                if (!handleSpecialCommands(userInput)) {
                    std::cout << "Unknown command: '" << userInput << "'\n";
                    std::cout << "Type 'help' for available commands.\n\n";
                }
            }
        }
    }

private:
    // ========== COMMAND HANDLERS ==========

    void handleInitialize() {
        std::cout << "\nInitializing system...\n";
        
        // Load configuration from file
        config = ConfigLoader::loadFromFile("config.txt");
        
        // Validate configuration
        if (!config.isValid()) {
            std::cout << "\nInitialization FAILED due to invalid configuration.\n";
            std::cout << "Please fix the errors in config.txt and try again.\n\n";
            return;
        }
        
        config.display();

        // Create MemoryManager using config (PAGING mode for MCO2)  // NEW
        if (memoryManager) {
            delete memoryManager;
            memoryManager = nullptr;
        }
        memoryManager = new MemoryManager(
            config.maxOverallMem,
            config.memPerFrame,
            config.minMemPerProc,
            config.maxMemPerProc,
            MemoryManager::PAGING,
            MemoryManager::FIRST_FIT
        );

        // Create scheduler and pass MemoryManager pointer            // CHANGED
        if (scheduler) {
            scheduler->stop();
            delete scheduler;
            scheduler = nullptr;
        }
        scheduler = new Scheduler(config, memoryManager);  // Pass memoryManager
        scheduler->start();
        
        cmdHandler.setSystemInitialized(true);
        std::cout << "System initialization complete!\n\n";
    }

    void handleScreenList() {
        if (scheduler) {
            scheduler->displayProcessLists();
        } else {
            std::cout << "ERROR: Scheduler not initialized.\n";
        }
    }

    void handleSchedulerStart() {
        if (scheduler) {
            scheduler->startProcessGeneration();
            std::cout << "\nAutomatic process generation started.\n";
            std::cout << "Processes will be created every " << config.batchProcessFreq << " seconds.\n\n";
        } else {
            std::cout << "ERROR: Scheduler not initialized.\n";
        }
    }

    void handleSchedulerStop() {
        if (scheduler) {
            scheduler->stopProcessGeneration();
            std::cout << "\nAutomatic process generation stopped.\n\n";
        } else {
            std::cout << "ERROR: Scheduler not initialized.\n";
        }
    }

    void handleReportUtil() {
        if (scheduler) {
            // Generate filename with timestamp
            std::string filename = "csopesy-log.txt";
            
            std::ofstream reportFile(filename);
            if (!reportFile.is_open()) {
                std::cout << "ERROR: Could not create report file.\n";
                return;
            }
            
            // Get current time for report header
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            std::string timestamp = std::ctime(&now_time);
            timestamp.pop_back(); // Remove newline
            
            // Write report header
            reportFile << "Report generated at: " << timestamp << "\n\n";
            
            // Write CPU utilization
            reportFile << "CPU Utilization: " << scheduler->getCPUUtilization() << "%\n";
            reportFile << "Cores used: " << scheduler->countActiveCoresPublic() << "/" << config.numCPUs << "\n";
            reportFile << "Cores available: " << (config.numCPUs - scheduler->countActiveCoresPublic()) << "/" << config.numCPUs << "\n\n";
            
            reportFile << "--------------------------------------\n\n";
            
            // Write running processes
            reportFile << "Running processes:\n";
            auto runningProcs = scheduler->getRunningProcesses();
            if (runningProcs.empty()) {
                reportFile << "(None)\n";
            } else {
                for (auto p : runningProcs) {
                    reportFile << p->getName() << " (" << p->getArrivalTime() << ")  Core: " 
                               << p->getAssignedCore() << "  " 
                               << p->getInstructionsExecuted() << "/" << p->getTotalInstructions() << "\n";
                }
            }
            reportFile << "\n";
            
            // Write finished processes
            reportFile << "Finished processes:\n";
            auto finishedProcs = scheduler->getFinishedProcesses();
            if (finishedProcs.empty()) {
                reportFile << "(None)\n";
            } else {
                for (auto p : finishedProcs) {
                    reportFile << p->getName() << " (" << p->getArrivalTime() << ")  Finished  " 
                               << p->getInstructionsExecuted() << "/" << p->getTotalInstructions() << "\n";
                }
            }
            reportFile << "\n";

            // NEW: append memory statistics snapshot
            if (memoryManager) {
                reportFile << "--------------------------------------\n\n";
                reportFile << memoryManager->getMemorySnapshot() << "\n";
            }
            
            reportFile << "--------------------------------------\n";
            
            reportFile.close();
            
            std::cout << "Report generated: " << filename << "\n";
        } else {
            std::cout << "ERROR: Scheduler not initialized.\n";
        }
    }

    void handleExit() {
        std::cout << "\nShutting down OS Simulator...\n";
        if (scheduler) {
            scheduler->stop();
        }
        std::cout << "Goodbye!\n";
        cmdHandler.stop();
    }

    // Enter interactive process screen
    void enterProcessScreen(const std::string& processName) {
        clearScreen();
        
        // Process screen loop
        bool inProcessScreen = true;
        while (inProcessScreen) {
            std::string command;
            std::cout << "root: ";
            std::getline(std::cin, command);
            
            if (command == "process-smi") {
                displayProcessSMI(processName);
            }
            else if (command == "exit") {
                inProcessScreen = false;
                clearScreen();
                displayBanner();
            }
            else if (command.empty()) {
                // Ignore empty input
                continue;
            }
            else {
                std::cout << "Unknown command. Available commands: process-smi, exit\n";
            }
        }
    }
    
    // Display process SMI (System Management Interface)
    void displayProcessSMI(const std::string& processName) {
        if (!scheduler) {
            std::cout << "ERROR: Scheduler not initialized.\n";
            return;
        }
        
        // If no process name given, show memory overview for all processes
        if (processName.empty()) {
            if (!memoryManager) {
                std::cout << "ERROR: Memory manager not initialized.\n";
                return;
            }
            
            std::cout << "\n========== PROCESS-SMI ==========\n\n";
            
            // Memory statistics
            std::cout << "Memory Usage:\n";
            std::cout << "  Total Memory: " << memoryManager->getMaxMemory() << " KB\n";
            std::cout << "  Used Memory: " << memoryManager->getUsedMemory() << " KB\n";
            std::cout << "  Free Memory: " << memoryManager->getFreeMemory() << " KB\n";
            std::cout << "  Utilization: " << std::fixed << std::setprecision(2) 
                      << memoryManager->getMemoryUtilization() << "%\n\n";
            
            // CPU statistics
            std::cout << "CPU Usage:\n";
            std::cout << "  Cores: " << config.numCPUs << "\n";
            std::cout << "  Utilization: " << std::fixed << std::setprecision(2) 
                      << scheduler->getCPUUtilization() << "%\n\n";
            
            // Running processes
            std::cout << "Running Processes:\n";
            auto running = scheduler->getRunningProcesses();
            if (running.empty()) {
                std::cout << "  (None)\n";
            } else {
                for (auto p : running) {
                    std::cout << "  " << p->getName() 
                              << " (PID " << p->getID() << ")"
                              << " - Core " << p->getAssignedCore()
                              << " - " << p->getMemoryRequired() << " KB"
                              << " - " << p->getInstructionsExecuted() << "/" 
                              << p->getTotalInstructions() << " instructions\n";
                }
            }
            std::cout << "\n";
            
            // Finished processes (last 5)
            std::cout << "Finished Processes (last 5):\n";
            auto finished = scheduler->getFinishedProcesses();
            if (finished.empty()) {
                std::cout << "  (None)\n";
            } else {
                int start = std::max(0, (int)finished.size() - 5);
                for (int i = start; i < (int)finished.size(); i++) {
                    auto p = finished[i];
                    std::cout << "  " << p->getName() 
                              << " (PID " << p->getID() << ")"
                              << " - " << p->getMemoryRequired() << " KB\n";
                }
            }
            
            std::cout << "\n==================================\n\n";
            return;
        }
        
        // Show specific process details
        Process* p = scheduler->findProcess(processName);
        if (!p) {
            std::cout << "Process '" << processName << "' not found.\n";
            return;
        }
        
        // Display process info
        std::cout << "\nProcess: " << p->getName();
        if (p->isFinished()) {
            std::cout << " (Finished!)";
        }
        std::cout << "\n";
        std::cout << "ID: " << p->getID() << "\n";
        
        // Display memory info
        if (memoryManager && memoryManager->isProcessAllocated(p->getID())) {
            auto memInfo = memoryManager->getProcessMemory(p->getID());
            std::cout << "Memory: " << memInfo.memoryAllocated << " KB allocated\n";
            std::cout << "Pages: " << memInfo.numPages << "\n";
        }
        
        // Display current instruction line and total
        std::cout << "\nCurrent instruction line: " << p->getInstructionsExecuted() << "\n";
        std::cout << "Lines of code: " << p->getTotalInstructions() << "\n";
        
        // Display logs
        std::cout << "\nLogs:\n";
        std::string logPath = p->getLogFilePath();
        if (!logPath.empty()) {
            std::ifstream logFile(logPath);
            if (logFile.is_open()) {
                std::string line;
                bool skipHeader = true;
                while (std::getline(logFile, line)) {
                    // Skip the first two lines (Process name and "Logs:" header)
                    if (skipHeader) {
                        if (line.find("Logs:") != std::string::npos) {
                            skipHeader = false;
                        }
                        continue;
                    }
                    std::cout << line << "\n";
                }
                logFile.close();
            } else {
                std::cout << "(No logs available yet)\n";
            }
        } else {
            std::cout << "(Log file not initialized)\n";
        }
        std::cout << "\n";
    }

    // Handle commands with parameters
    bool handleSpecialCommands(const std::string& input) {
        // Handle "screen -r ProcessName" - view specific process
        if (input.find("screen -r ") == 0) {
            std::string processName = input.substr(10);
            if (scheduler) {
                Process* p = scheduler->findProcess(processName);
                if (p) {
                    // Clear screen
                    clearScreen();
                    
                    // Display process info and logs
                    std::cout << "Process name: " << p->getName() << "\n";
                    std::cout << "ID: " << p->getID() << "\n";
                    std::cout << "Logs:\n";
                    
                    // Read and display the log file
                    std::string logPath = p->getLogFilePath();
                    if (!logPath.empty()) {
                        std::ifstream logFile(logPath);
                        if (logFile.is_open()) {
                            std::string line;
                            bool skipHeader = true;
                            while (std::getline(logFile, line)) {
                                // Skip the first two lines (Process name and "Logs:" header)
                                if (skipHeader) {
                                    if (line.find("Logs:") != std::string::npos) {
                                        skipHeader = false;
                                    }
                                    continue;
                                }
                                std::cout << line << "\n";
                            }
                            logFile.close();
                        } else {
                            std::cout << "(No logs available yet)\n";
                        }
                    } else {
                        std::cout << "(Log file not initialized)\n";
                    }
                    
                    // Display current status
                    std::cout << "\nCurrent instruction line: " << p->getInstructionsExecuted() << "\n";
                    std::cout << "Lines of code: " << p->getTotalInstructions() << "\n";
                    std::cout << "\n";
                } else {
                    std::cout << "Process '" << processName << "' not found.\n";
                }
            }
            return true;
        }
        
        
        // Handle "screen -s ProcessName" - create process and enter its screen
        if (input.find("screen -s ") == 0) {
            std::string args = input.substr(10);
            
            // Trim whitespace
            while (!args.empty() && args[0] == ' ') args = args.substr(1);
            while (!args.empty() && args[args.length()-1] == ' ') args = args.substr(0, args.length()-1);
            
            if (args.empty()) {
                std::cout << "Usage: screen -s <processname> <memory_size>\n";
                std::cout << "Memory size must be power of 2 in range [64, 65536] bytes\n";
                return true;
            }
            
            // Parse process name and memory size
            std::string name;
            size_t memSize = 0;
            
            size_t spacePos = args.find(' ');
            if (spacePos == std::string::npos) {
                // Missing memory parameter
                std::cout << "Usage: screen -s <processname> <memory_size>\n";
                std::cout << "Memory size must be power of 2 in range [64, 65536] bytes\n";
                return true;
            }
            
            // Extract name and memory string
            name = args.substr(0, spacePos);
            std::string memStr = args.substr(spacePos + 1);
            
            // Trim memory string
            while (!memStr.empty() && memStr[0] == ' ') memStr = memStr.substr(1);
            
            // Parse memory size
            try {
                memSize = std::stoi(memStr);
            } catch (...) {
                std::cout << "Invalid memory allocation. Memory size must be a number.\n";
                return true;
            }
            
            // Validate: Must be power of 2
            bool isPowerOf2 = (memSize > 0) && ((memSize & (memSize - 1)) == 0);
            if (!isPowerOf2) {
                std::cout << "Invalid memory allocation. Memory size must be a power of 2.\n";
                return true;
            }
            
            // Validate: Must be in range [64, 65536] = [2^6, 2^16]
            if (memSize < 64 || memSize > 65536) {
                std::cout << "Invalid memory allocation. Memory size must be between 64 and 65536 bytes.\n";
                return true;
            }
            
            if (scheduler) {
                // Generate random instruction count
                int instructions = config.minInstructions + 
                    (rand() % (config.maxInstructions - config.minInstructions + 1));
                
                Process* newProcess = new Process(
                    name,
                    scheduler->getTotalProcesses(),
                    instructions,
                    "Manual"
                );
                
                // Use the specified memory size (validated above)
                newProcess->setMemoryRequirement(memSize, config.memPerFrame);
        
                if (!memoryManager || 
                    !memoryManager->allocateMemory(newProcess->getID(), newProcess->getName(), memSize)) {
                    std::cout << "ERROR: Unable to allocate memory for process '" 
                              << name << "'. Not enough memory.\n";
                    delete newProcess;
                    return true;
                }
                
                // Generate instructions (VAR, PRINT, ADD pattern)
                newProcess->generateInstructions(instructions);
                
                // Initialize log file
                scheduler->initializeProcessLogPublic(newProcess);
                
                scheduler->addProcess(newProcess);
                
                // Enter the process screen
                enterProcessScreen(name);
            } else {
                std::cout << "ERROR: Scheduler not initialized.\n";
            }
            return true;
        }
        
        // Handle "screen -c ProcessName "instructions""
        if (input.find("screen -c ") == 0) {
            size_t firstQuote = input.find('"');
            size_t lastQuote = input.rfind('"');
            
            if (firstQuote == std::string::npos || lastQuote == std::string::npos || firstQuote == lastQuote) {
                std::cout << "Usage: screen -c <processname> \"instruction1; instruction2; ...\"\n";
                return true;
            }
            
            std::string beforeQuote = input.substr(10, firstQuote - 10);
            beforeQuote.erase(0, beforeQuote.find_first_not_of(" \t"));
            beforeQuote.erase(beforeQuote.find_last_not_of(" \t") + 1);
            
            std::string processName = beforeQuote;
            std::string instructionText = input.substr(firstQuote + 1, lastQuote - firstQuote - 1);
            
            if (processName.empty()) {
                std::cout << "Usage: screen -c <processname> \"instruction1; instruction2; ...\"\n";
                return true;
            }
            
            // Split instructions by semicolon
            std::vector<std::string> instructions;
            std::stringstream ss(instructionText);
            std::string line;
            while (std::getline(ss, line, ';')) {
                line.erase(0, line.find_first_not_of(" \t"));
                line.erase(line.find_last_not_of(" \t") + 1);
                if (!line.empty()) {
                    instructions.push_back(line);
                }
            }
            
            if (instructions.empty()) {
                std::cout << "ERROR: No valid instructions provided.\n";
                return true;
            }
            
            if (scheduler) {
                int memSize = 1024;  // Default size for custom processes
                
                Process* newProcess = new Process(
                    processName,
                    scheduler->getTotalProcesses(),
                    instructions.size(),
                    "Manual",
                    instructions  // Custom instructions constructor
                );
                
                newProcess->setMemoryRequirement(memSize, config.memPerFrame);
                
                if (!memoryManager || 
                    !memoryManager->allocateMemory(newProcess->getID(), newProcess->getName(), memSize)) {
                    std::cout << "ERROR: Unable to allocate memory for process '" << processName << "'.\n";
                    delete newProcess;
                    return true;
                }
                
                scheduler->initializeProcessLogPublic(newProcess);
                scheduler->addProcess(newProcess);
                
                std::cout << "Created custom process '" << processName << "' with " 
                          << instructions.size() << " instructions.\n";
                
                enterProcessScreen(processName);
            } else {
                std::cout << "ERROR: Scheduler not initialized.\n";
            }
            return true;
        }
        
        return false;
    }
};

#endif // MAIN_MENU_H