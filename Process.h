#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <ctime>
#include <cstdlib>
#include <cstdint>
#include <sstream>
#include <algorithm>
#include <filesystem>

// Page structure for memory management
struct Page {
    int pageNumber;
    bool isInMemory;      // True if page is currently in physical memory
    int frameNumber;      // Frame number if in memory, -1 otherwise
    bool isValid;         // For backward compatibility with reference
     
    Page(int num) : pageNumber(num), isInMemory(false), frameNumber(-1), isValid(false) {}
};

// Process - Represents a single process in the system
class Process {
public:
    enum ProcessState {
        READY,      // Waiting in queue
        RUNNING,    // Currently executing
        WAITING,    // Blocked (optional, for future use)
        FINISHED    // Completed execution
    };

private:
    std::string processName;
    int processID;
    ProcessState currentState;
    
    // Instruction tracking
    int totalInstructions;
    int instructionsExecuted;
    int remainingInstructions;
    
    // Instruction list (actual commands to execute)
    std::vector<std::string> instructions;
    
    // Process variables (for computation) - supports both simple and custom instructions
    int registerA;
    int registerB;
    int result;
    std::map<std::string, uint16_t> variables;  // For custom instructions (DECLARE, ADD, etc.)
    std::map<std::string, uint16_t> memory;     // Simulated memory for WRITE/READ
    
    // Sleep counter for SLEEP instruction
    int sleepCounter;
    
    // Flag for custom vs auto-generated instructions
    bool isCustom;
    
    // Timing information
    std::string arrivalTime;
    std::string startTime;
    std::string finishTime;
    
    // Core assignment (for multi-core simulation)
    int assignedCore;
    
    // Logging
    std::string logFilePath;
    std::vector<std::string> logs;

    // Memory Management
    std::vector<Page> pages;
    int memoryRequired;
    int numPages;

    // Helper method for getting variable values
    uint16_t getValue(const std::string& key) {
        if (variables.find(key) == variables.end()) {
            variables[key] = 0;
        }
        return variables[key];
    }

    // Get current timestamp
    std::string getCurrentTime() const {
        std::time_t now = std::time(nullptr);
        std::tm* ltm = std::localtime(&now);
        
        char buffer[20];
        std::snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d, %02d:%02d:%02d",
                     ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_year + 1900,
                     ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
        return std::string(buffer);
    }

public:
    // Constructor for manual/auto-generated processes
    Process(std::string name, int id, int instructionCount, std::string arrival)
        : processName(name), 
          processID(id),
          currentState(READY),
          totalInstructions(instructionCount),
          instructionsExecuted(0),
          remainingInstructions(instructionCount),
          registerA(0),
          registerB(0),
          result(0),
          sleepCounter(0),
          isCustom(false),
          arrivalTime(arrival),
          startTime(""),
          finishTime(""),
          assignedCore(-1),
          logFilePath(""),
          memoryRequired(0),
          numPages(0) {
        // Instructions will be generated separately
    }

    // Constructor for custom instruction processes
    Process(std::string name, int id, int instructionCount, std::string arrival,
            const std::vector<std::string>& customInstructions)
        : processName(name), 
          processID(id),
          currentState(READY),
          totalInstructions(instructionCount),
          instructionsExecuted(0),
          remainingInstructions(instructionCount),
          registerA(0),
          registerB(0),
          result(0),
          sleepCounter(0),
          isCustom(true),
          arrivalTime(arrival),
          startTime(""),
          finishTime(""),
          assignedCore(-1),
          logFilePath(""),
          memoryRequired(0),
          numPages(0) {
        generateSpecificInstructions(customInstructions);
    }

    // Getters
    std::string getName() const { return processName; }
    int getID() const { return processID; }
    ProcessState getState() const { return currentState; }
    int getTotalInstructions() const { return totalInstructions; }
    int getInstructionsExecuted() const { return instructionsExecuted; }
    int getRemainingInstructions() const { return remainingInstructions; }
    std::string getArrivalTime() const { return arrivalTime; }
    std::string getStartTime() const { return startTime; }
    std::string getFinishTime() const { return finishTime; }
    int getAssignedCore() const { return assignedCore; }
    std::string getLogFilePath() const { return logFilePath; }
    int getRegisterA() const { return registerA; }
    bool getIsCustom() const { return isCustom; }
    const std::vector<std::string>& getInstructions() const { return instructions; }

    // Setters
    void setState(ProcessState newState) { currentState = newState; }
    void setStartTime(std::string time) { startTime = time; }
    void setFinishTime(std::string time) { finishTime = time; }
    void setAssignedCore(int core) { assignedCore = core; }
    void setLogFilePath(std::string path) { logFilePath = path; }

    // Write a log entry to the process's log file
    void writeLog(const std::string& timestamp, int coreID, const std::string& message) {
        if (logFilePath.empty()) return;
        
        std::ofstream logFile(logFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << "(" << timestamp << ") Core:" << coreID 
                    << " \"" << message << "\"\n";
            logFile.close();
        }
    }

    // Add log message
    void addLog(const std::string& message) {
        std::string logEntry = getCurrentTime() + " " + message;
        logs.push_back(logEntry);
        
        // Write to log file
        if (!logFilePath.empty()) {
            std::ofstream file(logFilePath, std::ios::app);
            if (file.is_open()) {
                file << logEntry << "\n";
                file.close();
            }
        }
    }

    // Get current instruction (without executing)
    std::string getCurrentInstruction() const {
        if (instructionsExecuted < instructions.size()) {
            return instructions[instructionsExecuted];
        }
        return "";
    }

    // Print command (for PRINT instruction)
    void printCommand() {
        std::string msg = "Hello world from " + processName + "!";
        std::string logEntry = "(" + getCurrentTime() + ") Core:" + std::to_string(assignedCore) 
            + " \"" + msg + "\"";
        
        logs.push_back(logEntry);
        
        if (!logFilePath.empty()) {
            std::ofstream file(logFilePath, std::ios::app);
            if (file.is_open()) {
                file << logEntry << "\n";
                file.close();
            }
        }
    }

    // Execute one instruction
    void executeInstruction() {
        if (sleepCounter > 0) {
            sleepCounter--;
            return; // Skip this cycle while sleeping
        }

        if (instructionsExecuted >= instructions.size() || remainingInstructions <= 0) {
            return;
        }

        std::string instr = instructions[instructionsExecuted];

        if (!isCustom) {
            // Auto-generated instructions (simple pattern)
            if (instr.find("VAR") == 0) {
                // VAR X = value
                size_t equalPos = instr.find('=');
                if (equalPos != std::string::npos) {
                    registerA = std::stoi(instr.substr(equalPos + 1));
                }
            }
            else if (instr.find("ADD") == 0) {
                // ADD value
                size_t spacePos = instr.find(' ');
                if (spacePos != std::string::npos) {
                    int valueToAdd = std::stoi(instr.substr(spacePos + 1));
                    registerA += valueToAdd;
                }
            }
            else if (instr.find("PRINT") == 0) {
                printCommand();
            }
            else if (instr.find("DECLARE") == 0) {
                // DECLARE variable
                std::string var = "x";
                uint16_t val = rand() % 65536;
                variables[var] = val;
            }
            else if (instr.find("SUBTRACT") == 0) {
                std::string var1 = "x";
                std::string var2 = "x";
                uint16_t val2 = getValue(var2);
                uint16_t val3 = rand() % 10;
                int32_t result = static_cast<int32_t>(val2) - static_cast<int32_t>(val3);
                variables[var1] = static_cast<uint16_t>(std::max(0, result));
            }
            else if (instr.find("SLEEP") == 0) {
                sleepCounter = 1 + (rand() % 3);  // SLEEP 1-3 cycles
            }
            else if (instr.find("FOR") == 0) {
                // Insert PRINT instructions
                int remainingLines = totalInstructions - instructionsExecuted - 1;
                int maxInsert = std::min(remainingLines, 3);
                int loopCount = 1 + (rand() % (maxInsert + 1));
                
                for (int i = 0; i < loopCount; i++) {
                    instructionsExecuted++;
                    instructions.insert(instructions.begin() + instructionsExecuted, "PRINT");
                }
                return;
            }
            
        } else {
            // Custom instructions
            std::istringstream iss(instr);
            std::string command;
            iss >> command;

            if (command == "DECLARE") {
                std::string var;
                uint16_t value;
                iss >> var >> value;
                variables[var] = value;
            }
            else if (command == "ADD") {
                std::string dest, op1, op2;
                iss >> dest >> op1 >> op2;
                variables[dest] = getValue(op1) + getValue(op2);
            }
            else if (command == "SUBTRACT") {
                std::string dest, op1, op2;
                iss >> dest >> op1 >> op2;
                int32_t result = static_cast<int32_t>(getValue(op1)) - static_cast<int32_t>(getValue(op2));
                variables[dest] = static_cast<uint16_t>(std::max(0, result));
            }
            else if (command == "SLEEP") {
                int duration;
                iss >> duration;
                sleepCounter = std::max(1, duration);
            }
            else if (command == "WRITE") {
                std::string addressStr, var;
                iss >> addressStr >> var;
                uint16_t val = getValue(var);
                memory[addressStr] = val;
                addLog("[WRITE] " + addressStr + " <- " + std::to_string(val));
            }
            else if (command == "READ") {
                std::string var, addressStr;
                iss >> var >> addressStr;
                if (memory.find(addressStr) != memory.end()) {
                    variables[var] = memory[addressStr];
                } else {
                    variables[var] = 0;
                }
                addLog("[READ] " + var + " <- " + std::to_string(variables[var]) + " (from " + addressStr + ")");
            }
            else if (command == "PRINT") {
                // Get rest of line after PRINT
                std::string output;
                std::getline(iss, output);
                
                // Replace variables with their values
                for (auto it = variables.begin(); it != variables.end(); ++it) {
                    size_t pos = output.find(it->first);
                    if (pos != std::string::npos) {
                        output.replace(pos, it->first.length(), std::to_string(it->second));
                    }
                }
                
                // Clean up quotes and extra characters
                output.erase(std::remove(output.begin(), output.end(), '"'), output.end());
                output.erase(std::remove(output.begin(), output.end(), '+'), output.end());
                
                addLog(output);
                printCommand();
            }
        }

        instructionsExecuted++;
        remainingInstructions--;
    }

    // Generate instructions for this process (simple VAR/ADD/PRINT pattern)
    void generateInstructions(int count) {
        instructions.clear();
        isCustom = false;
        
        // First instruction: VAR X = 0
        int initialValue = 0;
        instructions.push_back("VAR X = " + std::to_string(initialValue));
        
        // Alternate between PRINT and ADD for remaining instructions
        for (int i = 1; i < count; i++) {
            if (i % 2 == 1) {
                instructions.push_back("PRINT \"Value from " + processName + "!\"");
            } else {
                int valueToAdd = rand() % 10 + 1;
                instructions.push_back("ADD " + std::to_string(valueToAdd));
            }
        }
    }

    // Generate random instructions (reference style)
    void generateRandomInstructions(int count) {
        static std::vector<std::string> types = {
            "PRINT", "ADD", "SUBTRACT", "DECLARE", "SLEEP", "FOR"
        };
        instructions.clear();
        isCustom = false;
        
        for (int i = 0; i < count; ++i) {
            int index = rand() % types.size();
            instructions.push_back(types[index]);
        }
    }

    // Generate specific instructions from user input
    void generateSpecificInstructions(const std::vector<std::string>& rawInstructions) {
        instructions.clear();
        isCustom = true;
        
        for (std::string instr : rawInstructions) {
            // Trim whitespace
            instr.erase(0, instr.find_first_not_of(" \t"));
            instr.erase(instr.find_last_not_of(" \t") + 1);

            static std::vector<std::string> valid = {
                "PRINT", "ADD", "SUBTRACT", "DECLARE", "SLEEP", "FOR", "WRITE", "READ"
            };
            bool isValid = std::any_of(valid.begin(), valid.end(), [&](const std::string& cmd) {
                return instr.find(cmd) == 0;
            });

            if (isValid) {
                instructions.push_back(instr);
            } else {
                addLog("Warning: Ignoring unknown instruction: " + instr);
            }
        }
        
        totalInstructions = instructions.size();
        remainingInstructions = instructions.size();
    }

    // Check if process is complete
    bool isFinished() const {
        return remainingInstructions <= 0 || instructionsExecuted >= instructions.size();
    }

    // Get progress percentage
    float getProgress() const {
        if (totalInstructions == 0) return 100.0f;
        return (float)instructionsExecuted / totalInstructions * 100.0f;
    }

    // Get state as string
    std::string getStateString() const {
        switch (currentState) {
            case READY: return "Ready";
            case RUNNING: return "Running";
            case WAITING: return "Waiting";
            case FINISHED: return "Finished";
            default: return "Unknown";
        }
    }

    // Display process information
    void displayInfo() const {
        std::cout << "Process: " << processName << "\n";
        std::cout << "ID: " << processID << "\n";
        std::cout << "State: " << getStateString() << "\n";
        std::cout << "Instructions: " << instructionsExecuted << "/" << totalInstructions << "\n";
        std::cout << "Progress: " << getProgress() << "%\n";
        
        if (!arrivalTime.empty())
            std::cout << "Arrival Time: " << arrivalTime << "\n";
        if (!startTime.empty())
            std::cout << "Start Time: " << startTime << "\n";
        if (!finishTime.empty())
            std::cout << "Finish Time: " << finishTime << "\n";
        if (assignedCore >= 0)
            std::cout << "Core: " << assignedCore << "\n";
    }

    // Display compact info (one line)
    void displayCompact() const {
        std::cout << processName 
                  << " | Core: " << (assignedCore >= 0 ? std::to_string(assignedCore) : "N/A")
                  << " | " << instructionsExecuted << "/" << totalInstructions
                  << " | " << getStateString() << "\n";
    }

    // === MEMORY MANAGEMENT METHODS ===

    void setMemoryRequirement(int memKB, int frameSize) {
        memoryRequired = memKB;
        numPages = (memKB + frameSize - 1) / frameSize;  // Ceiling division
        pages.clear();
        for (int i = 0; i < numPages; i++) {
            pages.push_back(Page(i));
        }
    }
     
    int getMemoryRequired() const { return memoryRequired; }
    int getNumPages() const { return numPages; }
    std::vector<Page>& getPages() { return pages; }
    const std::vector<Page>& getPages() const { return pages; }
    
    bool hasPageInMemory(int pageNum) const {
        if (pageNum >= 0 && pageNum < (int)pages.size()) {
            return pages[pageNum].isInMemory;
        }
        return false;
    }

    void generateRandomPages(int pageCount) {
        pages.clear();
        numPages = pageCount;
        for (int i = 0; i < pageCount; ++i) {
            pages.push_back(Page(i));
        }
    }

    void printPages() const {
        std::cout << "Process: " << processName << " | Pages: \n";
        for (const auto& page : pages) {
            std::cout << "  Page #" << page.pageNumber
                << " | In Memory: " << (page.isInMemory ? "Yes" : "No")
                << " | Frame: " << page.frameNumber << '\n';
        }
        std::cout << std::endl;
    }
};

#endif // PROCESS_H