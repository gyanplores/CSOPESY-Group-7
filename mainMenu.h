#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <iostream>
#include <string>
#include <cstdlib>
#include <sstream>
#include "CommandHandler.h"
#include "Config.h"
#include "Scheduler.h"

class MainMenu {
private:
    CommandHandler cmdHandler;
    bool clearOnCommand;
    Scheduler* scheduler;
    SystemConfig config;
    
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
                cmd == "vmstat" ||
                cmd == "process-smi");
    }

public:
    MainMenu() : clearOnCommand(false) {}

    // Setup all commands
    void setupCommands() {
        // Initialize command
        cmdHandler.registerCommand("initialize", [this]() {
            handleInitialize();
        });

        // Screen-ls command
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

        // Report-util
        cmdHandler.registerCommand("report-util", [this]() {
            handleReportUtil();
        });

        // VMSTAT command
        cmdHandler.registerCommand("vmstat", [this]() {
            handleVMStat();
        });

        // Process SMI command
        cmdHandler.registerCommand("process-smi", [this]() {
            handleProcessSMI();
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
    void handleInitialize() {
        std::cout << "\nInitializing system...\n";
        
        config = ConfigLoader::loadFromFile("config.txt");
        config.display();

        scheduler = new Scheduler(config);
        scheduler->start();
        
        std::cout << "Configuration loaded successfully.\n";
        std::cout << "Scheduler initialized.\n";
        //std::cout << "Memory manager initialized.\n";
        
        cmdHandler.setSystemInitialized(true);
        std::cout << "\nSystem initialization complete!\n\n";
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
            scheduler->displayUtilizationReport();
        } else {
            std::cout << "ERROR: Scheduler not initialized.\n";
        }
    }
    
    void handleVMStat() {
        std::cout << "\n=== Virtual Memory Statistics ===\n";
        // TODO: Display memory statistics
        std::cout << "Total Memory: 1024 MB\n";
        std::cout << "Used Memory: 0 MB\n";
        std::cout << "Free Memory: 1024 MB\n";
        std::cout << "=================================\n\n";
    } 

    void handleProcessSMI() {
        std::cout << "\n=== Process System Management Interface ===\n";
        // TODO: Display detailed process information
        std::cout << "No processes currently running.\n";
        std::cout << "===========================================\n\n";
    }

    void handleExit() {
        std::cout << "\nShutting down OS Simulator...\n";
        if (scheduler) {
            scheduler->stop();
        }
        std::cout << "Goodbye!\n";
        cmdHandler.stop();
    }

    // Handle commands with parameters
    bool handleSpecialCommands(const std::string& input) {
        // Handle "screen -r ProcessName" - view specific process
        if (input.find("screen -r ") == 0) {
            std::string processName = input.substr(10);
            if (scheduler) {
                Process* p = scheduler->findProcess(processName);
                if (p) {
                    std::cout << "\n";
                    p->displayInfo();
                    std::cout << "\n";
                } else {
                    std::cout << "Process '" << processName << "' not found.\n";
                }
            }
            return true;
        }
        
        // Handle "screen -s ProcessName Instructions" - create custom process
        if (input.find("screen -s ") == 0) {
            std::istringstream iss(input.substr(10));
            std::string name;
            int instructions;
            
            if (iss >> name >> instructions) {
                if (scheduler) {
                    Process* newProcess = new Process(
                        name,
                        scheduler->getTotalProcesses(),
                        instructions,
                        "Manual"
                    );
                    scheduler->addProcess(newProcess);
                    std::cout << "Created process: " << name << " with " 
                              << instructions << " instructions.\n";
                } else {
                    std::cout << "ERROR: Scheduler not initialized.\n";
                }
            } else {
                std::cout << "Usage: screen -s <name> <instructions>\n";
            }
            return true;
        }
        
        return false;
    }
};

#endif