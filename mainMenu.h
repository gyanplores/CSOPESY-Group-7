#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <iostream>
#include <string>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>
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
    }

    // Main menu loop
    void run() {
        clearScreen();
        displayBanner();
        
        std::cout << "Type 'help' to see available commands.\n";
        std::cout << "Type 'initialize' to set up the system.\n\n";

        while (cmdHandler.shouldContinue()) {
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
            std::string name = input.substr(10);
            
            // Trim whitespace
            while (!name.empty() && name[0] == ' ') name = name.substr(1);
            while (!name.empty() && name[name.length()-1] == ' ') name = name.substr(0, name.length()-1);
            
            if (name.empty()) {
                std::cout << "Usage: screen -s <processname>\n";
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
                
                // NEW: choose a memory size in [minMemPerProc, maxMemPerProc]
                size_t memSize = config.minMemPerProc;
                if (config.maxMemPerProc > config.minMemPerProc) {
                    memSize = config.minMemPerProc + 
                        (rand() % (static_cast<int>(config.maxMemPerProc - config.minMemPerProc + 1)));
                }

                // NEW: allocate memory for this process
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
        
        return false;
    }
};

#endif // MAIN_MENU_H
