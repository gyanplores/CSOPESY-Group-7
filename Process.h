#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <cstdlib>

struct Page {
    int pageNumber;     
    bool isInMemory;
    int frameNumber;
     
    Page(int num) : pageNumber(num), isInMemory(false), frameNumber(-1) {}
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
    
    // Process variables (for computation)
    int registerA;
    int registerB;
    int result;
    
    // Timing information
    std::string arrivalTime;
    std::string startTime;
    std::string finishTime;
    
    // Core assignment (for multi-core simulation)
    int assignedCore;
    
    // Logging
    std::string logFilePath;

    // Memory Management
    std::vector<Page> pages;
    int memoryRequired;
    int numPages;

public:
    // Constructor
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
          arrivalTime(arrival),
          startTime(""),
          finishTime(""),
          assignedCore(-1),
          logFilePath(""),
          memoryRequired(0),
          numPages(0) {
        // Instructions will be generated separately
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

    // Get current instruction (without executing)
    std::string getCurrentInstruction() const {
        if (instructionsExecuted < instructions.size()) {
            return instructions[instructionsExecuted];
        }
        return "";
    }

    // Execute one instruction
    void executeInstruction() {
        if (remainingInstructions > 0 && instructionsExecuted < instructions.size()) {
            std::string instruction = instructions[instructionsExecuted];
            
            // Parse and execute the instruction
            if (instruction.find("VAR") == 0) {
                // VAR X = value
                size_t equalPos = instruction.find('=');
                if (equalPos != std::string::npos) {
                    registerA = std::stoi(instruction.substr(equalPos + 1));
                }
            }
            else if (instruction.find("ADD") == 0) {
                // ADD value
                size_t spacePos = instruction.find(' ');
                if (spacePos != std::string::npos) {
                    int valueToAdd = std::stoi(instruction.substr(spacePos + 1));
                    registerA += valueToAdd;
                }
            }
            // PRINT doesn't need execution logic, just logged
            
            instructionsExecuted++;
            remainingInstructions--;
        }
    }

    // Generate instructions for this process
    void generateInstructions(int count) {
        instructions.clear();
        
        // First instruction: VAR X = <random>
        int initialValue = 0;  // initialize to 0
        instructions.push_back("VAR X = " + std::to_string(initialValue));
        
        // Alternate between PRINT and ADD for remaining instructions
        for (int i = 1; i < count; i++) {
            if (i % 2 == 1) {
                // Odd positions: PRINT
                instructions.push_back("PRINT \"Value from " + processName + "!\"");
            } else {
                // Even positions: ADD
                int valueToAdd = rand() % 10 + 1;  // Random 1-10
                instructions.push_back("ADD " + std::to_string(valueToAdd));
            }
        }
    }

    // Get current value of X
    int getRegisterA() const { return registerA; }

    // Check if process is complete
    bool isFinished() const {
        return remainingInstructions <= 0;
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

    // MEMORY MANAGEMENT METHODS

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
        if (pageNum >= 0 && pageNum < pages.size()) {
            return pages[pageNum].isInMemory;
        }
        return false;
    }
};

#endif // PROCESS_H
