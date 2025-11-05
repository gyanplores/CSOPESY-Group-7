#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <string>
#include <iostream>
#include <map>
#include <functional>


class CommandHandler {
private:
    // Map of command names to their handler functions
    std::map<std::string, std::function<void()>> commands;
    bool isRunning;
    bool systemInitialized;

public:
    CommandHandler() : isRunning(true), systemInitialized(false) {}

    // Register a new command with its handler function
    void registerCommand(const std::string& cmdName, std::function<void()> handler) {
        commands[cmdName] = handler;
    }

    // Check if system is initialized before allowing certain commands
    bool isSystemReady() const {
        return systemInitialized;
    }

    void setSystemInitialized(bool status) {
        systemInitialized = status;
    }

    // Execute a command by name
    bool executeCommand(const std::string& input) {
        // Check if command exists
        if (commands.find(input) != commands.end()) {
            commands[input]();
            return true;
        }
        return false;
    }

    // Stop the command loop
    void stop() {
        isRunning = false;
    }

    bool shouldContinue() const {
        return isRunning;
    }

    // Display available commands
    void showHelp() {
        std::cout << "\n=== Available Commands ===\n";
        for (const auto& cmd : commands) {
            std::cout << "  - " << cmd.first << "\n";
        }
        std::cout << "==========================\n\n";
    }
};

#endif // COMMAND_HANDLER_H

