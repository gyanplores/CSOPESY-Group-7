#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <set>
#include "Process.h"
#include "Config.h"

// CPU Core - Represents a single CPU core
class CPUCore {
private:
    int coreID;
    Process* currentProcess;
    bool isIdle;
    int executedCycles;
    int delayCyclesRemaining;  // For busy-waiting

public:
    CPUCore(int id) : coreID(id), currentProcess(nullptr), isIdle(true), executedCycles(0), delayCyclesRemaining(0) {}

    bool idle() const { return isIdle; }
    int getID() const { return coreID; }
    Process* getProcess() const { return currentProcess; }
    int getExecutedCycles() const { return executedCycles; }
    int getDelayCyclesRemaining() const { return delayCyclesRemaining; }

    void assignProcess(Process* p) {
        currentProcess = p;
        isIdle = false;
        executedCycles = 0;
        delayCyclesRemaining = 0;
        if (p) {
            p->setAssignedCore(coreID);
            p->setState(Process::RUNNING);
        }
    }

    void releaseProcess() {
        if (currentProcess) {
            currentProcess->setAssignedCore(-1);
        }
        currentProcess = nullptr;
        isIdle = true;
        executedCycles = 0;
        delayCyclesRemaining = 0;
    }

    // Execute one cycle (either busy-waiting or actual instruction)
    void executeCycle(int delayPerExec) {
        if (currentProcess && !isIdle) {
            if (delayCyclesRemaining > 0) {
                // Busy-waiting - process stays in CPU but doesn't execute instruction
                delayCyclesRemaining--;
            } else {
                // Execute the actual instruction
                currentProcess->executeInstruction();
                executedCycles++;
                
                // Set up delay cycles for next instruction (if any)
                if (!currentProcess->isFinished() && delayPerExec > 0) {
                    delayCyclesRemaining = delayPerExec;
                }
            }
        }
    }

    bool processFinished() const {
        return currentProcess && currentProcess->isFinished();
    }
    
    bool isBusyWaiting() const {
        return delayCyclesRemaining > 0;
    }
};

/**
 * Scheduler - Manages process scheduling and CPU cores
 */
class Scheduler {
private:
    // Configuration
    SystemConfig config;
    
    // CPU Cores
    std::vector<CPUCore*> cpuCores;
    
    // Process Queues
    std::queue<Process*> readyQueue;
    std::vector<Process*> runningProcesses;
    std::vector<Process*> finishedProcesses;
    
    // Thread control
    std::atomic<bool> isRunning;
    std::atomic<bool> autoGenerateProcesses;
    std::mutex queueMutex;
    std::mutex runningMutex;
    std::mutex finishedMutex;
    
    // Statistics
    int totalProcessesCreated;
    int currentCycle;
    std::chrono::steady_clock::time_point startTime;

public:
    Scheduler(const SystemConfig& cfg) 
        : config(cfg),
          isRunning(false),
          autoGenerateProcesses(false),
          totalProcessesCreated(0),
          currentCycle(0) {
        
        // Create CPU cores
        for (int i = 0; i < config.numCPUs; i++) {
            cpuCores.push_back(new CPUCore(i));
        }
    }

    ~Scheduler() {
        stop();
        for (auto core : cpuCores) {
            delete core;
        }
        // Clean up processes
        for (auto p : runningProcesses) delete p;
        for (auto p : finishedProcesses) delete p;
        while (!readyQueue.empty()) {
            delete readyQueue.front();
            readyQueue.pop();
        }
    }

    // Add a process to the ready queue
    void addProcess(Process* process) {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(process);
    }

    // Start the scheduler
    void start() {
        if (!isRunning) {
            isRunning = true;
            startTime = std::chrono::steady_clock::now();
            
            // Start CPU execution thread
            std::thread executionThread(&Scheduler::cpuExecutionLoop, this);
            executionThread.detach();
        }
    }

    // Stop the scheduler
    void stop() {
        isRunning = false;
        autoGenerateProcesses = false;
    }

    // Start automatic process generation
    void startProcessGeneration() {
        if (!autoGenerateProcesses) {
            autoGenerateProcesses = true;
            std::thread genThread(&Scheduler::processGenerationLoop, this);
            genThread.detach();
        }
    }

    // Stop automatic process generation
    void stopProcessGeneration() {
        autoGenerateProcesses = false;
    }

    // Get statistics
    int getTotalProcesses() const { return totalProcessesCreated; }
    int getReadyQueueSize() const { return readyQueue.size(); }
    int getRunningCount() const { return runningProcesses.size(); }
    int getFinishedCount() const { return finishedProcesses.size(); }
    int getCurrentCycle() const { return currentCycle; }

    // Calculate CPU utilization
    float getCPUUtilization() const {
        int busyCores = 0;
        for (auto core : cpuCores) {
            if (!core->idle()) busyCores++;
        }
        return (float)busyCores / cpuCores.size() * 100.0f;
    }

    // Display process lists
    void displayProcessLists() {
        std::cout << "\n========== PROCESS STATUS ==========\n\n";
        
        // Running processes
        std::cout << "Running Processes:\n";
        {
            std::lock_guard<std::mutex> lock(runningMutex);
            if (runningProcesses.empty()) {
                std::cout << "  (None)\n";
            } else {
                for (auto p : runningProcesses) {
                    std::cout << "  ";
                    p->displayCompact();
                }
            }
        }
        std::cout << "\n";

        // Ready queue
        std::cout << "Ready Queue (Size: " << readyQueue.size() << "):\n";
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (readyQueue.empty()) {
                std::cout << "  (Empty)\n";
            } else {
                // Can't iterate queue directly, so just show count
                std::cout << "  " << readyQueue.size() << " processes waiting\n";
            }
        }
        std::cout << "\n";

        // Finished processes
        std::cout << "Finished Processes (Total: " << finishedProcesses.size() << "):\n";
        {
            std::lock_guard<std::mutex> lock(finishedMutex);
            if (finishedProcesses.empty()) {
                std::cout << "  (None)\n";
            } else {
                int showCount = std::min(10, (int)finishedProcesses.size());
                for (int i = finishedProcesses.size() - showCount; i < finishedProcesses.size(); i++) {
                    std::cout << "  ";
                    finishedProcesses[i]->displayCompact();
                }
                if (finishedProcesses.size() > 10) {
                    std::cout << "  ... (showing last 10)\n";
                }
            }
        }
        std::cout << "\n====================================\n\n";
    }

    // Display detailed report
    void displayUtilizationReport() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        
        std::cout << "\n========== UTILIZATION REPORT ==========\n";
        std::cout << "CPU Utilization: " << getCPUUtilization() << "%\n";
        std::cout << "Cores Used: " << countActiveCores() << "/" << config.numCPUs << "\n";
        std::cout << "Running Time: " << elapsed << " seconds\n";
        std::cout << "Current Cycle: " << currentCycle << "\n";
        std::cout << "\nProcess Statistics:\n";
        std::cout << "  Total Created: " << totalProcessesCreated << "\n";
        std::cout << "  Currently Running: " << getRunningCount() << "\n";
        std::cout << "  In Ready Queue: " << getReadyQueueSize() << "\n";
        std::cout << "  Finished: " << getFinishedCount() << "\n";
        std::cout << "========================================\n\n";
    }

    // Find process by name
    Process* findProcess(const std::string& name) {
        // Check running processes
        {
            std::lock_guard<std::mutex> lock(runningMutex);
            for (auto p : runningProcesses) {
                if (p->getName() == name) return p;
            }
        }
        
        // Check finished processes
        {
            std::lock_guard<std::mutex> lock(finishedMutex);
            for (auto p : finishedProcesses) {
                if (p->getName() == name) return p;
            }
        }
        
        return nullptr;
    }

    // Public method to initialize process log (for manually created processes)
    void initializeProcessLogPublic(Process* process) {
        initializeProcessLog(process);
    }
    
    // Get running processes (for report)
    std::vector<Process*> getRunningProcesses() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(runningMutex));
        return runningProcesses;
    }
    
    // Get finished processes (for report)
    std::vector<Process*> getFinishedProcesses() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(finishedMutex));
        return finishedProcesses;
    }
    
    // Get active core count (for report)
    int countActiveCoresPublic() const {
        return countActiveCores();
    }

    // ========== MEMORY DEALLOCATION (NEW) ==========
    
    // This will be called from MainMenu to deallocate finished processes
    void deallocateFinishedProcesses(MemoryManager* memMgr) {
        if (!memMgr) return;
        
        std::lock_guard<std::mutex> lock(finishedMutex);
        
        // Track which processes we've deallocated
        static std::set<int> deallocatedProcesses;
        
        for (auto p : finishedProcesses) {
            // Only deallocate once per process
            if (deallocatedProcesses.find(p->getID()) == deallocatedProcesses.end()) {
                memMgr->deallocateMemory(p->getID());
                deallocatedProcesses.insert(p->getID());
            }
        }
    }

private:
    // Count active CPU cores
    int countActiveCores() const {
        int count = 0;
        for (auto core : cpuCores) {
            if (!core->idle()) count++;
        }
        return count;
    }

    // Create directory if it doesn't exist (cross-platform using system command)
    void createDirectoryIfNotExists(const std::string& path) {
        #ifdef _WIN32
            system("if not exist logs mkdir logs");
        #else
            system("mkdir -p logs");
        #endif
    }

    // Get formatted timestamp (MM/DD/YYYY, HH:MM:SS AM/PM)
    std::string getFormattedTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm* local_time = std::localtime(&now_time);
        
        std::ostringstream oss;
        // Format: MM/DD/YYYY, HH:MM:SS AM/PM
        oss << std::setfill('0') << std::setw(2) << (local_time->tm_mon + 1) << "/"
            << std::setfill('0') << std::setw(2) << local_time->tm_mday << "/"
            << (local_time->tm_year + 1900) << ", ";
        
        int hour = local_time->tm_hour;
        bool isPM = hour >= 12;
        if (hour == 0) hour = 12;
        else if (hour > 12) hour -= 12;
        
        oss << std::setfill('0') << std::setw(2) << hour << ":"
            << std::setfill('0') << std::setw(2) << local_time->tm_min << ":"
            << std::setfill('0') << std::setw(2) << local_time->tm_sec
            << (isPM ? " PM" : " AM");
        
        return oss.str();
    }

    // Initialize process log file
    void initializeProcessLog(Process* process) {
        // Create logs directory
        createDirectoryIfNotExists("logs");
        
        // Set log file path
        std::string logPath = "logs/" + process->getName() + ".txt";
        process->setLogFilePath(logPath);
        
        // Create/clear the log file
        std::ofstream logFile(logPath);
        if (logFile.is_open()) {
            logFile << "Process: " << process->getName() << "\n";
            logFile << "Logs:\n";
            logFile.close();
        }
    }

    // Main CPU execution loop
    void cpuExecutionLoop() {
        while (isRunning) {
            currentCycle++;
            
            // Assign processes to idle cores
            assignProcessesToCores();
            
            // Execute one cycle on all cores
            for (auto core : cpuCores) {
                if (!core->idle()) {
                    Process* p = core->getProcess();
                    
                    if (p && !core->isBusyWaiting()) {
                        // Only log when actually executing an instruction (not busy-waiting)
                        std::string instruction = p->getCurrentInstruction();
                        
                        // Execute the instruction (updates registers) with delay
                        core->executeCycle(config.delayPerExec);
                        
                        // Write log entry only for actual instruction execution
                        if (!instruction.empty()) {
                            std::string timestamp = getFormattedTimestamp();
                            std::string logMessage = instruction;
                            
                            // For ADD and VAR, show the result/value of X
                            if (instruction.find("ADD") == 0 || instruction.find("VAR") == 0) {
                                logMessage += " | X = " + std::to_string(p->getRegisterA());
                            }
                            
                            p->writeLog(timestamp, core->getID(), logMessage);
                        }
                    } else if (p && core->isBusyWaiting()) {
                        // Just busy-wait, don't execute instruction
                        core->executeCycle(config.delayPerExec);
                    }
                    
                    // Check if process finished
                    if (core->processFinished()) {
                        moveToFinished(core);
                    }
                    // Check for preemption (Round Robin)
                    else if (config.schedulerType == "rr" && 
                             core->getExecutedCycles() >= config.quantumCycles) {
                        preemptProcess(core);
                    }
                }
            }
            
            // Fixed 100ms per CPU cycle
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // Assign processes from ready queue to idle cores
    void assignProcessesToCores() {
        for (auto core : cpuCores) {
            if (core->idle()) {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (!readyQueue.empty()) {
                    Process* p = readyQueue.front();
                    readyQueue.pop();
                    
                    // Set start time if first time running
                    if (p->getStartTime().empty()) {
                        p->setStartTime(getCurrentTimeString());
                    }
                    
                    core->assignProcess(p);
                    
                    {
                        std::lock_guard<std::mutex> runLock(runningMutex);
                        runningProcesses.push_back(p);
                    }
                }
            }
        }
    }

    // Move finished process from core
    void moveToFinished(CPUCore* core) {
        Process* p = core->getProcess();
        if (p) {
            p->setState(Process::FINISHED);
            p->setFinishTime(getCurrentTimeString());
            
            {
                std::lock_guard<std::mutex> lock(finishedMutex);
                finishedProcesses.push_back(p);
            }
            
            {
                std::lock_guard<std::mutex> lock(runningMutex);
                runningProcesses.erase(
                    std::remove(runningProcesses.begin(), runningProcesses.end(), p),
                    runningProcesses.end()
                );
            }
            
            core->releaseProcess();
        }
    }

    // Preempt process (Round Robin)
    void preemptProcess(CPUCore* core) {
        Process* p = core->getProcess();
        if (p && !p->isFinished()) {
            p->setState(Process::READY);
            
            {
                std::lock_guard<std::mutex> lock(runningMutex);
                runningProcesses.erase(
                    std::remove(runningProcesses.begin(), runningProcesses.end(), p),
                    runningProcesses.end()
                );
            }
            
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                readyQueue.push(p);
            }
            
            core->releaseProcess();
        }
    }

    // Automatic process generation loop
    void processGenerationLoop() {
        while (autoGenerateProcesses) {
            // Wait based on batch frequency
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config.batchProcessFreq * 1000)
            );
            
            if (!autoGenerateProcesses) break;
            
            // Generate random instruction count
            int instructions = config.minInstructions + 
                (rand() % (config.maxInstructions - config.minInstructions + 1));
            
            // Create new process
            std::string name = "Process_" + std::to_string(totalProcessesCreated);
            Process* newProcess = new Process(
                name,
                totalProcessesCreated,
                instructions,
                getCurrentTimeString()
            );
            
            // Generate instructions (VAR, PRINT, ADD pattern)
            newProcess->generateInstructions(instructions);
            
            // Initialize log file for this process
            initializeProcessLog(newProcess);
            
            totalProcessesCreated++;
            addProcess(newProcess);
        }
    }

    // Get current time as string
    std::string getCurrentTimeString() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::string timeStr = std::ctime(&now_time);
        timeStr.pop_back(); // Remove newline
        return timeStr;
    }
};

#endif // SCHEDULER_H