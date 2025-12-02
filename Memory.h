#ifndef MEMORY_H
#define MEMORY_H

#include <vector>
#include <map>
#include <mutex>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <cstddef>  // For SIZE_MAX
#include <fstream>  // For backing store I/O

/**
 * Memory Management System with Paging
 * 
 * Supports:
 * - Flat memory allocation
 * - Paging memory allocation
 * - First-fit, best-fit, worst-fit allocation strategies
 * - Memory tracking per process
 * - Fragmentation statistics
 */

// Forward declaration
class Process;

// Memory Frame - Represents a single frame in physical memory
struct MemoryFrame {
    int frameNumber;
    bool isFree;
    int processID;           // -1 if free
    std::string processName;
    size_t size;             // Size of data in this frame
    std::time_t timestamp;   // When allocated
    
    MemoryFrame(int num) 
        : frameNumber(num), isFree(true), processID(-1), 
          processName(""), size(0), timestamp(0) {}
};

// Memory Block - For flat memory allocation
struct MemoryBlock {
    size_t startAddress;
    size_t size;
    bool isFree;
    int processID;           // -1 if free
    std::string processName;
    std::time_t timestamp;
    
    MemoryBlock(size_t start, size_t sz) 
        : startAddress(start), size(sz), isFree(true), 
          processID(-1), processName(""), timestamp(0) {}
};

// Process Memory Info
struct ProcessMemoryInfo {
    int processID;
    std::string processName;
    size_t memoryRequired;   // Total memory needed
    size_t memoryAllocated;  // Actual memory allocated
    std::vector<int> frameNumbers;  // For paging: assigned frames
    size_t startAddress;     // For flat: start address
    std::time_t allocationTime;
    int numPages;            // Number of pages allocated
    
    ProcessMemoryInfo() 
        : processID(-1), processName(""), memoryRequired(0), 
          memoryAllocated(0), startAddress(0), allocationTime(0), numPages(0) {}
};

class MemoryManager {
public:
    enum AllocationType {
        FLAT,     // Contiguous memory allocation
        PAGING    // Paged memory allocation
    };
    
    enum AllocationStrategy {
        FIRST_FIT,
        BEST_FIT,
        WORST_FIT
    };

private:
    // Configuration
    size_t maxMemorySize;           // Total memory in KB
    size_t memPerFrame;             // Memory per frame in KB (for paging)
    size_t minMemPerProcess;        // Minimum memory per process in KB
    size_t maxMemPerProcess;        // Maximum memory per process in KB
    
    AllocationType allocationType;
    AllocationStrategy allocationStrategy;
    
    // Memory structures
    std::vector<MemoryFrame> frames;        // For paging
    std::vector<MemoryBlock> blocks;        // For flat allocation
    std::map<int, ProcessMemoryInfo> processMemoryMap;  // ProcessID -> Memory info
    
    // Statistics
    size_t totalAllocated;
    size_t totalFree;
    int totalProcessesAllocated;
    int allocationFailures;

    // Backing store + paging stats
    std::string backingStorePath;
    size_t pagesPagedIn;
    size_t pagesPagedOut;
    
    // Thread safety
    mutable std::mutex memoryMutex;
    
    // Helper methods
    void initializeFlatMemory();
    void initializePaging();
    void mergeFreeBlocks();  // For flat allocation
    int findFirstFitBlock(size_t size);
    int findBestFitBlock(size_t size);
    int findWorstFitBlock(size_t size);

    // Backing store helpers
    void initializeBackingStoreFile();
    bool writeFrameToBackingStore(int frameNumber);
    
public:
    MemoryManager(size_t maxMem, size_t memFrame, size_t minMem, size_t maxMemProc, 
                  AllocationType type, AllocationStrategy strategy,
                  const std::string& backingPath = "csopesy-backing-store.txt")
        : maxMemorySize(maxMem),
          memPerFrame(memFrame),
          minMemPerProcess(minMem),
          maxMemPerProcess(maxMemProc),
          allocationType(type),
          allocationStrategy(strategy),
          totalAllocated(0),
          totalFree(maxMem),
          totalProcessesAllocated(0),
          allocationFailures(0),
          backingStorePath(backingPath),
          pagesPagedIn(0),
          pagesPagedOut(0) {
        
        if (allocationType == PAGING) {
            initializePaging();
        } else {
            initializeFlatMemory();
        }

        // Initialize / truncate backing store file
        initializeBackingStoreFile();
    }
    
    // Allocation methods
    bool allocateMemory(int processID, std::string processName, size_t memorySize);
    bool deallocateMemory(int processID);
    
    // Query methods
    size_t getTotalMemory() const { return maxMemorySize; }
    size_t getUsedMemory() const { return totalAllocated; }
    size_t getFreeMemory() const { return totalFree; }
    size_t getNumFreeFrames() const;
    size_t getNumUsedFrames() const;
    int getNumPages() const { return frames.size(); }
    
    // Getters for process-smi
    size_t getMaxMemory() const { return maxMemorySize; }
    size_t getTotalAllocated() const { return totalAllocated; }
    size_t getTotalFree() const { return totalFree; }

    // Process-specific queries
    bool isProcessAllocated(int processID) const;
    ProcessMemoryInfo getProcessMemory(int processID) const;
    
    // Statistics
    double getMemoryUtilization() const {
        if (maxMemorySize == 0) return 0.0;
        return (totalAllocated * 100.0) / maxMemorySize;
    }
    
    int getTotalProcesses() const { return totalProcessesAllocated; }
    int getAllocationFailures() const { return allocationFailures; }
    
    size_t getExternalFragmentation() const;  // For flat allocation
    size_t getInternalFragmentation() const;  // For paging
    
    // Display methods
    void displayMemoryMap() const;
    void displayVMStat() const;
    std::string getMemorySnapshot() const;
    
    // Configuration getters
    AllocationType getAllocationType() const { return allocationType; }
    AllocationStrategy getAllocationStrategy() const { return allocationStrategy; }
    size_t getMemPerFrame() const { return memPerFrame; }
    size_t getMinMemPerProcess() const { return minMemPerProcess; }
    size_t getMaxMemPerProcess() const { return maxMemPerProcess; }
};

// Initialize flat memory allocation
void MemoryManager::initializeFlatMemory() {
    blocks.clear();
    blocks.push_back(MemoryBlock(0, maxMemorySize));
}

// Initialize paging
void MemoryManager::initializePaging() {
    frames.clear();
    int numFrames = maxMemorySize / memPerFrame;
    for (int i = 0; i < numFrames; i++) {
        frames.push_back(MemoryFrame(i));
    }
}

// Initialize backing store file (truncate and write header)
void MemoryManager::initializeBackingStoreFile() {
    std::ofstream ofs(backingStorePath, std::ios::trunc);
    if (!ofs.is_open()) {
        std::cerr << "WARNING: Could not initialize backing store file at '"
                  << backingStorePath << "'.\n";
        return;
    }

    ofs << "CSOPESY Backing Store\n";
    ofs << "FrameSizeKB " << memPerFrame << "\n";
    ofs << "MaxMemoryKB " << maxMemorySize << "\n\n";
    ofs.close();
}

// Allocate memory for a process
bool MemoryManager::allocateMemory(int processID, std::string processName, size_t memorySize) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    
    // Check if process already allocated
    if (processMemoryMap.find(processID) != processMemoryMap.end()) {
        return false;  // Already allocated
    }
    
    // Clamp memory size to allowed range
    if (memorySize < minMemPerProcess) memorySize = minMemPerProcess;
    if (memorySize > maxMemPerProcess) memorySize = maxMemPerProcess;
    
    ProcessMemoryInfo memInfo;
    memInfo.processID = processID;
    memInfo.processName = processName;
    memInfo.memoryRequired = memorySize;
    memInfo.allocationTime = std::time(nullptr);
    
    if (allocationType == PAGING) {
        // Paging allocation
        int pagesNeeded = (memorySize + memPerFrame - 1) / memPerFrame;  // Ceiling division
        
        // Find free frames
        std::vector<int> freeFrames;
        for (auto& frame : frames) {
            if (frame.isFree) {
                freeFrames.push_back(frame.frameNumber);
                if ((int)freeFrames.size() >= pagesNeeded) break;
            }
        }
        
        if ((int)freeFrames.size() < pagesNeeded) {
            allocationFailures++;
            return false;  // Not enough frames
        }
        
        // Allocate frames
        for (int i = 0; i < pagesNeeded; i++) {
            int frameNum = freeFrames[i];
            frames[frameNum].isFree = false;
            frames[frameNum].processID = processID;
            frames[frameNum].processName = processName;
            frames[frameNum].size = (i == pagesNeeded - 1) ? 
                (memorySize - i * memPerFrame) : memPerFrame;
            frames[frameNum].timestamp = std::time(nullptr);
            
            memInfo.frameNumbers.push_back(frameNum);
        }
        
        memInfo.memoryAllocated = pagesNeeded * memPerFrame;
        memInfo.numPages = pagesNeeded;
        memInfo.startAddress = 0;  // Not applicable for paging
        
    } else {
        // Flat allocation
        int blockIndex = -1;
        
        // Find suitable block based on strategy
        if (allocationStrategy == FIRST_FIT) {
            blockIndex = findFirstFitBlock(memorySize);
        } else if (allocationStrategy == BEST_FIT) {
            blockIndex = findBestFitBlock(memorySize);
        } else {  // WORST_FIT
            blockIndex = findWorstFitBlock(memorySize);
        }
        
        if (blockIndex == -1) {
            allocationFailures++;
            return false;  // No suitable block found
        }
        
        // Allocate the block
        MemoryBlock& block = blocks[blockIndex];
        
        memInfo.startAddress = block.startAddress;
        memInfo.memoryAllocated = memorySize;
        memInfo.numPages = 0;  // Not applicable for flat
        
        // Split block if larger than needed
        if (block.size > memorySize) {
            MemoryBlock newBlock(block.startAddress + memorySize, block.size - memorySize);
            blocks.insert(blocks.begin() + blockIndex + 1, newBlock);
        }
        
        block.size = memorySize;
        block.isFree = false;
        block.processID = processID;
        block.processName = processName;
        block.timestamp = std::time(nullptr);
    }
    
    // Update statistics
    totalAllocated += memInfo.memoryAllocated;
    totalFree -= memInfo.memoryAllocated;
    totalProcessesAllocated++;
    
    // Store process memory info
    processMemoryMap[processID] = memInfo;
    
    return true;
}

// Write a frame's metadata to the backing store file
bool MemoryManager::writeFrameToBackingStore(int frameNumber) {
    if (allocationType != PAGING) return false;
    if (frameNumber < 0 || frameNumber >= (int)frames.size()) return false;

    const MemoryFrame& frame = frames[frameNumber];
    if (frame.isFree) return false;  // Nothing to write

    std::ofstream ofs(backingStorePath, std::ios::app);
    if (!ofs.is_open()) {
        std::cerr << "WARNING: Could not open backing store file '"
                  << backingStorePath << "' for appending.\n";
        return false;
    }

    std::time_t now = std::time(nullptr);
    std::string ts = std::ctime(&now);
    if (!ts.empty() && ts.back() == '\n') ts.pop_back();

    ofs << "FRAME " << frame.frameNumber
        << " PID " << frame.processID
        << " NAME " << frame.processName
        << " SIZEKB " << frame.size
        << " TIME " << ts << "\n";

    ofs.close();

    pagesPagedOut++;  // Count as page-out
    return true;
}

// Deallocate memory for a process
bool MemoryManager::deallocateMemory(int processID) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    
    auto it = processMemoryMap.find(processID);
    if (it == processMemoryMap.end()) {
        return false;  // Process not found
    }
    
    ProcessMemoryInfo& memInfo = it->second;
    
    if (allocationType == PAGING) {
        // Free all frames, but write to backing store first
        for (int frameNum : memInfo.frameNumbers) {
            // Simulate paging out to backing store on process termination
            writeFrameToBackingStore(frameNum);

            frames[frameNum].isFree = true;
            frames[frameNum].processID = -1;
            frames[frameNum].processName = "";
            frames[frameNum].size = 0;
        }
    } else {
        // Free the block
        for (auto& block : blocks) {
            if (block.processID == processID) {
                block.isFree = true;
                block.processID = -1;
                block.processName = "";
                break;
            }
        }
        
        // Merge adjacent free blocks
        mergeFreeBlocks();
    }
    
    // Update statistics
    totalAllocated -= memInfo.memoryAllocated;
    totalFree += memInfo.memoryAllocated;
    totalProcessesAllocated--;
    
    // Remove from map
    processMemoryMap.erase(it);
    
    return true;
}

// First-fit allocation strategy
int MemoryManager::findFirstFitBlock(size_t size) {
    for (size_t i = 0; i < blocks.size(); i++) {
        if (blocks[i].isFree && blocks[i].size >= size) {
            return (int)i;
        }
    }
    return -1;
}

// Best-fit allocation strategy
int MemoryManager::findBestFitBlock(size_t size) {
    int bestIndex = -1;
    size_t bestSize = SIZE_MAX;
    
    for (size_t i = 0; i < blocks.size(); i++) {
        if (blocks[i].isFree && blocks[i].size >= size && blocks[i].size < bestSize) {
            bestIndex = (int)i;
            bestSize = blocks[i].size;
        }
    }
    
    return bestIndex;
}

// Worst-fit allocation strategy
int MemoryManager::findWorstFitBlock(size_t size) {
    int worstIndex = -1;
    size_t worstSize = 0;
    
    for (size_t i = 0; i < blocks.size(); i++) {
        if (blocks[i].isFree && blocks[i].size >= size && blocks[i].size > worstSize) {
            worstIndex = (int)i;
            worstSize = blocks[i].size;
        }
    }
    
    return worstIndex;
}

// Merge adjacent free blocks (for flat allocation)
void MemoryManager::mergeFreeBlocks() {
    if (blocks.empty()) return;

    for (size_t i = 0; i < blocks.size() - 1; ) {
        if (blocks[i].isFree && blocks[i + 1].isFree) {
            blocks[i].size += blocks[i + 1].size;
            blocks.erase(blocks.begin() + i + 1);
        } else {
            i++;
        }
    }
}

// Get number of free frames
size_t MemoryManager::getNumFreeFrames() const {
    if (allocationType != PAGING) return 0;
    
    size_t count = 0;
    for (const auto& frame : frames) {
        if (frame.isFree) count++;
    }
    return count;
}

// Get number of used frames
size_t MemoryManager::getNumUsedFrames() const {
    if (allocationType != PAGING) return 0;
    
    return frames.size() - getNumFreeFrames();
}

// Check if process has memory allocated
bool MemoryManager::isProcessAllocated(int processID) const {
    std::lock_guard<std::mutex> lock(memoryMutex);
    return processMemoryMap.find(processID) != processMemoryMap.end();
}

// Get process memory info
ProcessMemoryInfo MemoryManager::getProcessMemory(int processID) const {
    std::lock_guard<std::mutex> lock(memoryMutex);
    
    auto it = processMemoryMap.find(processID);
    if (it != processMemoryMap.end()) {
        return it->second;
    }
    return ProcessMemoryInfo();
}

// Calculate external fragmentation (flat allocation)
size_t MemoryManager::getExternalFragmentation() const {
    if (allocationType != FLAT) return 0;
    
    size_t totalFreeInBlocks = 0;
    size_t largestFreeBlock = 0;
    
    for (const auto& block : blocks) {
        if (block.isFree) {
            totalFreeInBlocks += block.size;
            if (block.size > largestFreeBlock) {
                largestFreeBlock = block.size;
            }
        }
    }
    
    if (totalFreeInBlocks == 0) return 0;
    if (largestFreeBlock > totalFreeInBlocks) return 0;

    return totalFreeInBlocks - largestFreeBlock;
}

// Calculate internal fragmentation (paging)
size_t MemoryManager::getInternalFragmentation() const {
    if (allocationType != PAGING) return 0;
    
    size_t totalInternal = 0;
    
    for (const auto& pair : processMemoryMap) {
        const ProcessMemoryInfo& memInfo = pair.second;
        size_t allocated = memInfo.memoryAllocated;
        size_t required = memInfo.memoryRequired;
        if (allocated > required) {
            totalInternal += (allocated - required);
        }
    }
    
    return totalInternal;
}

// Display memory map
void MemoryManager::displayMemoryMap() const {
    std::lock_guard<std::mutex> lock(memoryMutex);
    
    std::cout << "\n========== MEMORY MAP ==========\n";
    
    if (allocationType == PAGING) {
        std::cout << "Allocation Type: Paging\n";
        std::cout << "Frame Size: " << memPerFrame << " KB\n";
        std::cout << "Total Frames: " << frames.size() << "\n";
        std::cout << "Used Frames: " << getNumUsedFrames() << "\n";
        std::cout << "Free Frames: " << getNumFreeFrames() << "\n\n";
        
        // Display frame allocation (first 20)
        for (size_t i = 0; i < frames.size() && i < 20; i++) {
            const auto& frame = frames[i];
            std::cout << "Frame " << std::setw(3) << frame.frameNumber << ": ";
            if (frame.isFree) {
                std::cout << "[FREE]\n";
            } else {
                std::cout << "[" << frame.processName << " (PID:" << frame.processID << ")]\n";
            }
        }
        if (frames.size() > 20) {
            std::cout << "... (showing first 20 of " << frames.size() << " frames)\n";
        }
    } else {
        std::cout << "Allocation Type: Flat\n";
        std::cout << "Strategy: ";
        if (allocationStrategy == FIRST_FIT) std::cout << "First-Fit\n";
        else if (allocationStrategy == BEST_FIT) std::cout << "Best-Fit\n";
        else std::cout << "Worst-Fit\n";
        
        std::cout << "\nMemory Blocks:\n";
        for (const auto& block : blocks) {
            std::cout << "Address " << std::setw(6) << block.startAddress 
                      << " - " << std::setw(6) << (block.startAddress + block.size - 1)
                      << " (" << std::setw(5) << block.size << " KB): ";
            if (block.isFree) {
                std::cout << "[FREE]\n";
            } else {
                std::cout << "[" << block.processName << " (PID:" << block.processID << ")]\n";
            }
        }
    }
    
    std::cout << "================================\n\n";
}

// Display vmstat
void MemoryManager::displayVMStat() const {
    std::lock_guard<std::mutex> lock(memoryMutex);
    
    std::cout << "\n========================================\n";
    std::cout << "VM STATISTICS\n";
    std::cout << "========================================\n\n";
    
    // Memory Overview
    std::cout << "Memory Overview:\n";
    std::cout << "Total Memory: " << maxMemorySize << " KB\n";
    std::cout << "Used Memory: " << totalAllocated << " KB\n";
    std::cout << "Free Memory: " << totalFree << " KB\n";
    std::cout << "Utilization: " << std::fixed << std::setprecision(2) 
              << getMemoryUtilization() << "%\n\n";
    
    // Process Statistics
    std::cout << "Process Statistics:\n";
    std::cout << "Active Processes: " << totalProcessesAllocated << "\n";
    std::cout << "Allocation Failures: " << allocationFailures << "\n\n";
    
    // Paging Information
    if (allocationType == PAGING) {
        std::cout << "Paging Information:\n";
        std::cout << "Total Frames: " << frames.size() << "\n";
        std::cout << "Used Frames: " << getNumUsedFrames() << "\n";
        std::cout << "Free Frames: " << getNumFreeFrames() << "\n";
        std::cout << "Frame Size: " << memPerFrame << " KB\n";
        std::cout << "Pages Paged In: " << pagesPagedIn << "\n";
        std::cout << "Pages Paged Out: " << pagesPagedOut << "\n";
        std::cout << "Internal Fragmentation: " << std::fixed << std::setprecision(2)
                  << getInternalFragmentation() << " KB\n\n";
        
        // Process Memory Allocations
        if (totalProcessesAllocated > 0) {
            std::cout << "Memory Allocations:\n";
            std::cout << "PID\tProcess Name\t\tFrames\tMemory (KB)\n";
            std::cout << "---\t------------\t\t------\t-----------\n";
            
            // Count frames per process
            std::map<int, int> frameCount;
            std::map<int, std::string> processNames;
            std::map<int, size_t> processMemory;
            
            for (const auto& frame : frames) {
                if (!frame.isFree) {
                    frameCount[frame.processID]++;
                    processNames[frame.processID] = frame.processName;
                    processMemory[frame.processID] += frame.size;
                }
            }
            
            for (const auto& entry : frameCount) {
                int pid = entry.first;
                int count = entry.second;
                std::string name = processNames[pid];
                size_t memKB = processMemory[pid];
                
                // Truncate long names
                if (name.length() > 20) {
                    name = name.substr(0, 17) + "...";
                }
                
                std::cout << pid << "\t" << std::left << std::setw(20) << name << std::right
                          << "\t" << count << "\t" << memKB << "\n";
            }
            std::cout << "\n";
        } else {
            std::cout << "No processes currently allocated in memory.\n\n";
        }
    } else {
        // Flat memory allocation
        std::cout << "Memory Blocks: " << blocks.size() << "\n";
        std::cout << "External Fragmentation: " << std::fixed << std::setprecision(2)
                  << getExternalFragmentation() << " KB\n\n";
    }
    
    std::cout << "========================================\n\n";
}

// Get memory snapshot for report
std::string MemoryManager::getMemorySnapshot() const {
    std::ostringstream oss;
    
    oss << "Memory Statistics:\n";
    oss << "Total Memory: " << maxMemorySize << " KB\n";
    oss << "Used Memory: " << totalAllocated << " KB\n";
    oss << "Free Memory: " << totalFree << " KB\n";
    oss << "Utilization: " << std::fixed << std::setprecision(2) 
        << getMemoryUtilization() << "%\n";
    oss << "Active Processes: " << totalProcessesAllocated << "\n";
    
    if (allocationType == PAGING) {
        oss << "Pages Used: " << getNumUsedFrames() << "/" << frames.size() << "\n";
        oss << "Pages Paged Out: " << pagesPagedOut << "\n";
        oss << "Pages Paged In: " << pagesPagedIn << "\n";
    }
    
    return oss.str();
}

#endif // MEMORY_H