#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <iostream>
#include <string>
#include <cstdlib>
#include "CommandHandler.h"

class mainMenu {
private:
    commandHandler cmdHandler;
    bool clearOnCommand;

    
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
    mainMenu() : clearOnCommand(false) {}

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
        // TODO: Load configuration from CONFIG.TXT
        // TODO: Initialize scheduler
        // TODO: Initialize memory manager
        
        std::cout << "Configuration loaded successfully.\n";
        std::cout << "Scheduler initialized.\n";
        std::cout << "Memory manager initialized.\n";
        
        cmdHandler.setSystemInitialized(true);
        std::cout << "\nSystem initialization complete!\n\n";
    }

    void handleScreenList() {
        std::cout << "\n=== Process List ===\n";
        // TODO: Display all processes
        std::cout << "Running Processes:\n";
        std::cout << "  (None)\n";
        std::cout << "\nReady Queue:\n";
        std::cout << "  (Empty)\n";
        std::cout << "\nFinished Processes:\n";
        std::cout << "  (None)\n";
        std::cout << "====================\n\n";
    }

    void handleSchedulerStart() {
        std::cout << "\nStarting automatic process generation...\n";
        // TODO: Start generating processes in background thread
        std::cout << "Scheduler is now creating sample processes.\n\n";
    }

    void handleSchedulerStop() {
        std::cout << "\nStopping automatic process generation...\n";
        // TODO: Stop the background thread
        std::cout << "Process generation stopped.\n\n";
    }

    void handleReportUtil() {
        std::cout << "\n=== CPU Utilization Report ===\n";
        // TODO: Calculate and display CPU statistics
        std::cout << "CPU Usage: 0.00%\n";
        std::cout << "Active Cores: 0/4\n";
        std::cout << "Total Instructions Executed: 0\n";
        std::cout << "==============================\n\n";
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
        std::cout << "Goodbye!\n";
        cmdHandler.stop();
    }

    // Handle commands with parameters
    bool handleSpecialCommands(const std::string& input) {
        // TODO: Parse commands like "screen -r ProcessName"
        // TODO: Parse commands like "screen -s ProcessName 100"
        
        // Example structure:
        if (input.substr(0, 9) == "screen -r") {
            std::cout << "Attaching to process screen...\n";
            return true;
        }
        
        if (input.substr(0, 9) == "screen -s") {
            std::cout << "Creating new process...\n";
            return true;
        }
        
        return false;
    }
};

#endif