#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

// Configuration structure for the OS Simulator
struct SystemConfig {
    // CPU Configuration
    int numCPUs;
    
    // Scheduler Configuration
    std::string schedulerType;  // "fcfs" or "rr"
    int quantumCycles;          // For Round Robin
    int batchProcessFreq;       // How often to generate processes
    
    // Process Configuration
    int minInstructions;
    int maxInstructions;
    int delayPerExec;          // CPU cycles to wait before next instruction (0-2^32)
    
    // Memory Configuration
    size_t maxOverallMem;      // Total memory in KB
    size_t memPerFrame;        // Memory per frame in KB (for paging)
    size_t minMemPerProc;      // Minimum memory per process in KB
    size_t maxMemPerProc;      // Maximum memory per process in KB
    
    // Constructor with defaults
    SystemConfig() 
        : numCPUs(4),
          schedulerType("fcfs"),
          quantumCycles(5),
          batchProcessFreq(3),
          minInstructions(100),
          maxInstructions(1000),
          delayPerExec(0),      // Default: 0 (execute one instruction per cycle)
          maxOverallMem(1024),  // 1024 KB = 1 MB
          memPerFrame(16),      // 16 KB per frame
          minMemPerProc(16),    // Min 16 KB per process
          maxMemPerProc(128) {} // Max 128 KB per process

    // Display configuration
    void display() const {
        std::cout << "\n=== System Configuration ===\n";
        std::cout << "Number of CPUs: " << numCPUs << "\n";
        std::cout << "Scheduler Type: " << schedulerType << "\n";
        std::cout << "Quantum Cycles: " << quantumCycles << "\n";
        std::cout << "Batch Process Frequency: " << batchProcessFreq << "\n";
        std::cout << "Min Instructions: " << minInstructions << "\n";
        std::cout << "Max Instructions: " << maxInstructions << "\n";
        std::cout << "Delay per Exec: " << delayPerExec << " cycles\n";
        std::cout << "\n--- Memory Configuration ---\n";
        std::cout << "Maximum Memory: " << maxOverallMem << " KB\n";
        std::cout << "Memory per Frame: " << memPerFrame << " KB\n";
        std::cout << "Memory per Process: " << minMemPerProc << " - " << maxMemPerProc << " KB\n";
        std::cout << "============================\n\n";
    }

    // Validate configuration
    bool isValid() const {
        bool valid = true;
        
        // Validate scheduler type
        if (schedulerType != "fcfs" && schedulerType != "rr") {
            std::cerr << "ERROR: Invalid scheduler type '" << schedulerType << "'\n";
            std::cerr << "       Must be 'fcfs' or 'rr'\n";
            valid = false;
        }
        
        // Validate number of CPUs
        if (numCPUs < 1 || numCPUs > 128) {
            std::cerr << "ERROR: Invalid number of CPUs (" << numCPUs << ")\n";
            std::cerr << "       Must be between 1 and 128\n";
            valid = false;
        }
        
        // Validate quantum cycles (for RR)
        if (schedulerType == "rr" && quantumCycles < 1) {
            std::cerr << "ERROR: Invalid quantum cycles (" << quantumCycles << ")\n";
            std::cerr << "       Must be at least 1 for Round Robin\n";
            valid = false;
        }
        
        // Validate instruction range
        if (minInstructions < 1 || maxInstructions < minInstructions) {
            std::cerr << "ERROR: Invalid instruction range\n";
            std::cerr << "       Min: " << minInstructions << ", Max: " << maxInstructions << "\n";
            valid = false;
        }
        
        // Validate memory configuration
        if (maxOverallMem < 1) {
            std::cerr << "ERROR: Invalid max overall memory (" << maxOverallMem << " KB)\n";
            valid = false;
        }
        
        if (memPerFrame < 1 || memPerFrame > maxOverallMem) {
            std::cerr << "ERROR: Invalid memory per frame (" << memPerFrame << " KB)\n";
            valid = false;
        }
        
        if (minMemPerProc < 1 || maxMemPerProc < minMemPerProc) {
            std::cerr << "ERROR: Invalid memory per process range\n";
            std::cerr << "       Min: " << minMemPerProc << ", Max: " << maxMemPerProc << " KB\n";
            valid = false;
        }
        
        if (maxMemPerProc > maxOverallMem) {
            std::cerr << "ERROR: Max memory per process (" << maxMemPerProc 
                      << " KB) exceeds total memory (" << maxOverallMem << " KB)\n";
            valid = false;
        }
        
        return valid;
    }
};

// ConfigLoader - Loads configuration from file
class ConfigLoader {
public:
    static SystemConfig loadFromFile(const std::string& filename) {
        SystemConfig config;
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open config file '" << filename << "'. Using defaults.\n";
            return config;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;
            
            // Parse key-value pairs
            std::istringstream iss(line);
            std::string key, value;
            
            if (iss >> key >> value) {
                parseConfigValue(config, key, value);
            }
        }
        
        file.close();
        return config;
    }

private:
    static void parseConfigValue(SystemConfig& config, const std::string& key, const std::string& value) {
        if (key == "num-cpu" || key == "num_cpu") {
            config.numCPUs = std::stoi(value);
        }
        else if (key == "scheduler" || key == "scheduler-type") {
            // Convert to lowercase for comparison
            std::string lowerValue = value;
            for (char& c : lowerValue) {
                c = std::tolower(c);
            }
            config.schedulerType = lowerValue;
        }
        else if (key == "quantum-cycles" || key == "quantum_cycles") {
            config.quantumCycles = std::stoi(value);
        }
        else if (key == "batch-process-freq" || key == "batch_process_freq") {
            config.batchProcessFreq = std::stoi(value);
        }
        else if (key == "min-ins" || key == "min_instructions") {
            config.minInstructions = std::stoi(value);
        }
        else if (key == "max-ins" || key == "max_instructions") {
            config.maxInstructions = std::stoi(value);
        }
        else if (key == "delay-per-exec" || key == "delay_per_exec") {
            config.delayPerExec = std::stoi(value);
        }
        // Memory configuration
        else if (key == "max-overall-mem" || key == "max_overall_mem") {
            config.maxOverallMem = std::stoull(value);
        }
        else if (key == "mem-per-frame" || key == "mem_per_frame") {
            config.memPerFrame = std::stoull(value);
        }
        else if (key == "min-mem-per-proc" || key == "min_mem_per_proc") {
            config.minMemPerProc = std::stoull(value);
        }
        else if (key == "max-mem-per-proc" || key == "max_mem_per_proc") {
            config.maxMemPerProc = std::stoull(value);
        }
    }
};

#endif // CONFIG_H