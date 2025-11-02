#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <iostream>
#include <ctime>

/**
 * Process - Represents a single process in the system
 * Simplified version without memory management
 */
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
    
    // Timing information
    std::string arrivalTime;
    std::string startTime;
    std::string finishTime;
    
    // Core assignment (for multi-core simulation)
    int assignedCore;

public:
    // Constructor
    Process(std::string name, int id, int instructions, std::string arrival)
        : processName(name), 
          processID(id),
          currentState(READY),
          totalInstructions(instructions),
          instructionsExecuted(0),
          remainingInstructions(instructions),
          arrivalTime(arrival),
          startTime(""),
          finishTime(""),
          assignedCore(-1) {}

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

    // Setters
    void setState(ProcessState newState) { currentState = newState; }
    void setStartTime(std::string time) { startTime = time; }
    void setFinishTime(std::string time) { finishTime = time; }
    void setAssignedCore(int core) { assignedCore = core; }

    // Execute one instruction
    void executeInstruction() {
        if (remainingInstructions > 0) {
            instructionsExecuted++;
            remainingInstructions--;
        }
    }

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
};

#endif // PROCESS_H