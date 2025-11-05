#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

/**
 * Configuration structure for the OS Simulator
 */
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
    
    // Constructor with defaults
    SystemConfig() 
        : numCPUs(4),
          schedulerType("fcfs"),
          quantumCycles(5),
          batchProcessFreq(3),
          minInstructions(100),
          maxInstructions(1000),
          delayPerExec(0) {}  // Default: 0 (execute one instruction per cycle)

    // Display configuration
    void display() const {
        std::cout << "\n=== System Configuration ===\n";
        std::cout << "Number of CPUs: " << numCPUs << "\n";
        std::cout << "CPU Cycle Time: 100 ms (fixed)\n";
        std::cout << "Scheduler Type: " << schedulerType << "\n";
        std::cout << "Quantum Cycles: " << quantumCycles << "\n";
        std::cout << "Batch Process Frequency: " << batchProcessFreq << "\n";
        std::cout << "Min Instructions: " << minInstructions << "\n";
        std::cout << "Max Instructions: " << maxInstructions << "\n";
        std::cout << "Delay per Exec: " << delayPerExec << " cycles";
        if (delayPerExec == 0) {
            std::cout << " (1 instruction per cycle)";
        } else {
            std::cout << " (busy-wait " << delayPerExec << " cycles per instruction)";
        }
        std::cout << "\n============================\n\n";
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
        
        return valid;
    }
};

/**
 * ConfigLoader - Reads configuration from file
 */
class ConfigLoader {
public:
    static SystemConfig loadFromFile(const std::string& filename) {
        SystemConfig config;
        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cerr << "Warning: Could not open config file '" << filename 
                      << "'. Using default values.\n";
            return config;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

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
    }
};

#endif // CONFIG_H