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
#include <vector>
#include <iomanip>
#include <algorithm>
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
    MemoryManager* memoryManager;   // global memory manager

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

    // Helper: trim spaces from both ends
    std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t");
        return s.substr(start, end - start + 1);
    }

    // Helper: check if value is a power of two
    bool isPowerOfTwo(size_t x) {
        return x != 0 && (x & (x - 1)) == 0;
    }

public:
    MainMenu() 
        : clearOnCommand(false), 
          scheduler(nullptr),
          memoryManager(nullptr) {}

    ~MainMenu() {
        if (scheduler) {
            scheduler->stop();
            delete scheduler;
        }
        if (memoryManager) {
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
                // Handle special commands with parameters (like screen -r, screen -s, screen -c)
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

        // Create MemoryManager using config (PAGING mode for MCO2)
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

        // Create scheduler and pass MemoryManager pointer
        if (scheduler) {
            scheduler->stop();
            delete scheduler;
            scheduler = nullptr;
        }
        scheduler = new Scheduler(config, memoryManager);
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
            std::string filename = "csopesy-log.txt";
            
            std::ofstream reportFile(filename);
            if (!reportFile.is_open()) {
                std::cout << "ERROR: Could not create report file.\n";
                return;
            }
            
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            std::string timestamp = std::ctime(&now_time);
            timestamp.pop_back(); // Remove newline
            
            reportFile << "Report generated at: " << timestamp << "\n\n";
            
            reportFile << "CPU Utilization: " << scheduler->getCPUUtilization() << "%\n";
            reportFile << "Cores used: " << scheduler->countActiveCoresPublic() << "/" << config.numCPUs << "\n";
            reportFile << "Cores available: " << (config.numCPUs - scheduler->countActiveCoresPublic()) << "/" << config.numCPUs << "\n\n";
            
            reportFile << "--------------------------------------\n\n";
            
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
        
        // If no process name given, show memory/CPU overview
        if (processName.empty()) {
            if (!memoryManager) {
                std::cout << "ERROR: Memory manager not initialized.\n";
                return;
            }
            
            std::cout << "\n========== PROCESS-SMI ==========\n\n";
            
            std::cout << "Memory Usage:\n";
            std::cout << "  Total Memory: " << memoryManager->getTotalMemory() << " KB\n";
            std::cout << "  Used Memory: " << memoryManager->getUsedMemory() << " KB\n";
            std::cout << "  Free Memory: " << memoryManager->getFreeMemory() << " KB\n";
            std::cout << "  Utilization: " << std::fixed << std::setprecision(2) 
                      << memoryManager->getMemoryUtilization() << "%\n\n";
            
            std::cout << "CPU Usage:\n";
            std::cout << "  Cores: " << config.numCPUs << "\n";
            std::cout << "  Utilization: " << std::fixed << std::setprecision(2) 
                      << scheduler->getCPUUtilization() << "%\n\n";
            
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
        
        std::cout << "\nProcess: " << p->getName();
        if (p->isFinished()) {
            std::cout << " (Finished!)";
        }
        std::cout << "\n";
        std::cout << "ID: " << p->getID() << "\n";
        
        if (memoryManager && memoryManager->isProcessAllocated(p->getID())) {
            auto memInfo = memoryManager->getProcessMemory(p->getID());
            std::cout << "Memory: " << memInfo.memoryAllocated << " KB allocated\n";
            std::cout << "Pages: " << memInfo.numPages << "\n";
        }
        
        std::cout << "\nCurrent instruction line: " << p->getInstructionsExecuted() << "\n";
        std::cout << "Lines of code: " << p->getTotalInstructions() << "\n";
        
        std::cout << "\nLogs:\n";
        std::string logPath = p->getLogFilePath();
        if (!logPath.empty()) {
            std::ifstream logFile(logPath);
            if (logFile.is_open()) {
                std::string line;
                bool skipHeader = true;
                while (std::getline(logFile, line)) {
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
        // ---------- screen -c ----------
        // Format: screen -c <name> <mem_size> "instr1; instr2; ..."
        if (input.rfind("screen -c ", 0) == 0) {
            std::string rest = input.substr(10); // after "screen -c "

            // Find first quote (start of instructions)
            size_t firstQuote = rest.find('"');
            if (firstQuote == std::string::npos) {
                std::cout << "invalid command\n";
                return true;
            }

            // Text before quote: "<name> <mem_size>"
            std::string beforeQuote = trim(rest.substr(0, firstQuote));

            // Text inside quotes: instructions
            std::string afterQuote = rest.substr(firstQuote + 1);
            size_t lastQuote = afterQuote.find('"');
            if (lastQuote == std::string::npos) {
                std::cout << "invalid command\n";
                return true;
            }
            std::string instructionsPart = afterQuote.substr(0, lastQuote);

            // Parse name + mem_size
            std::istringstream iss(beforeQuote);
            std::string processName;
            std::string memStr;
            iss >> processName >> memStr;

            if (processName.empty() || memStr.empty()) {
                std::cout << "invalid command\n";
                return true;
            }

            long long memSize = 0;
            try {
                memSize = std::stoll(memStr);
            } catch (...) {
                std::cout << "invalid memory allocation\n";
                return true;
            }

            // Memory: power of 2, between 64 and 65536
            if (memSize <= 0 ||
                !isPowerOfTwo(static_cast<size_t>(memSize)) ||
                memSize < 64 || memSize > 65536) {
                std::cout << "invalid memory allocation\n";
                return true;
            }

            // Split instructions by ';'
            std::vector<std::string> instructions;
            std::stringstream ss(instructionsPart);
            std::string line;
            while (std::getline(ss, line, ';')) {
                line = trim(line);
                if (!line.empty()) instructions.push_back(line);
            }

            // 1–50 instructions allowed
            if (instructions.empty() || instructions.size() > 50) {
                std::cout << "invalid command\n";
                return true;
            }

            if (!scheduler) {
                std::cout << "ERROR: Scheduler not initialized.\n";
                return true;
            }

            // Use custom-instruction constructor from Process.h
            Process* newProcess = new Process(
                processName,
                scheduler->getTotalProcesses(),
                static_cast<int>(instructions.size()),
                "Manual",
                instructions
            );

            newProcess->setMemoryRequirement((int)memSize, (int)config.memPerFrame);

            if (!memoryManager ||
                !memoryManager->allocateMemory(newProcess->getID(), newProcess->getName(), memSize)) {
                std::cout << "ERROR: Unable to allocate memory for process '" 
                          << processName << "'.\n";
                delete newProcess;
                return true;
            }

            scheduler->initializeProcessLogPublic(newProcess);
            scheduler->addProcess(newProcess);

            std::cout << "Created custom process '" << processName
                      << "' with " << instructions.size() << " instructions.\n";

            enterProcessScreen(processName);
            return true;
        }

        // ---------- screen -r ----------
        if (input.find("screen -r ") == 0) {
            std::string processName = input.substr(10);
            if (scheduler) {
                Process* p = scheduler->findProcess(processName);
                if (p) {
                    clearScreen();
                    std::cout << "Process name: " << p->getName() << "\n";
                    std::cout << "ID: " << p->getID() << "\n";
                    std::cout << "Logs:\n";
                    
                    std::string logPath = p->getLogFilePath();
                    if (!logPath.empty()) {
                        std::ifstream logFile(logPath);
                        if (logFile.is_open()) {
                            std::string line;
                            bool skipHeader = true;
                            while (std::getline(logFile, line)) {
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
                    
                    std::cout << "\nCurrent instruction line: " << p->getInstructionsExecuted() << "\n";
                    std::cout << "Lines of code: " << p->getTotalInstructions() << "\n";
                    std::cout << "\n";
                } else {
                    std::cout << "Process '" << processName << "' not found.\n";
                }
            }
            return true;
        }
        
        
        // ---------- screen -s ----------
        if (input.find("screen -s ") == 0) {
            std::string args = input.substr(10);
            
            while (!args.empty() && args[0] == ' ') args = args.substr(1);
            while (!args.empty() && args[args.length()-1] == ' ') args = args.substr(0, args.length()-1);
            
            if (args.empty()) {
                std::cout << "Usage: screen -s <processname> <memory_size>\n";
                std::cout << "Memory size must be power of 2 in range [64, 65536] bytes\n";
                return true;
            }
            
            std::string name;
            size_t memSize = 0;
            
            size_t spacePos = args.find(' ');
            if (spacePos == std::string::npos) {
                std::cout << "Usage: screen -s <processname> <memory_size>\n";
                std::cout << "Memory size must be power of 2 in range [64, 65536] bytes\n";
                return true;
            }
            
            name = args.substr(0, spacePos);
            std::string memStr = args.substr(spacePos + 1);
            
            while (!memStr.empty() && memStr[0] == ' ') memStr = memStr.substr(1);
            
            try {
                memSize = std::stoi(memStr);
            } catch (...) {
                std::cout << "Invalid memory allocation. Memory size must be a number.\n";
                return true;
            }
            
            bool isPower2 = (memSize > 0) && ((memSize & (memSize - 1)) == 0);
            if (!isPower2) {
                std::cout << "Invalid memory allocation. Memory size must be a power of 2.\n";
                return true;
            }
            
            if (memSize < 64 || memSize > 65536) {
                std::cout << "Invalid memory allocation. Memory size must be between 64 and 65536 bytes.\n";
                return true;
            }
            
            if (scheduler) {
                int instructions = config.minInstructions + 
                    (rand() % (config.maxInstructions - config.minInstructions + 1));
                
                Process* newProcess = new Process(
                    name,
                    scheduler->getTotalProcesses(),
                    instructions,
                    "Manual"
                );
                
                newProcess->setMemoryRequirement((int)memSize, (int)config.memPerFrame);
        
                if (!memoryManager || 
                    !memoryManager->allocateMemory(newProcess->getID(), newProcess->getName(), memSize)) {
                    std::cout << "ERROR: Unable to allocate memory for process '" 
                              << name << "'. Not enough memory.\n";
                    delete newProcess;
                    return true;
                }
                
                newProcess->generateInstructions(instructions);
                scheduler->initializeProcessLogPublic(newProcess);
                scheduler->addProcess(newProcess);
                
                enterProcessScreen(name);
            } else {
                std::cout << "ERROR: Scheduler not initialized.\n";
            }
            return true;
        }
        
        return false;
    }
};

#endif // MAIN_MENU_H
