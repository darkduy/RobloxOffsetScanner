#pragma once
#include "memory_scanner.h"
#include <map>
#include <set>

class DynamicFinder {
public:
    struct Result {
        std::string name;
        uintptr_t offset;
        double confidence;
        bool found;
        
        Result() : offset(0), confidence(0.0), found(false) {}
        Result(const std::string& n) : name(n), offset(0), confidence(0.0), found(false) {}
    };

private:
    MemoryScanner& scanner;
    std::vector<Result> results;

public:
    DynamicFinder(MemoryScanner& s) : scanner(s) {
        results = {
            Result("OFF_MAP_END"),
            Result("OFF_MAP_LIST"),
            Result("OFF_MAP_MASK"),
            Result("OFF_ENTRY_FORWARD"),
            Result("OFF_ENTRY_STRING"),
            Result("OFF_ENTRY_GETSET"),
            Result("OFF_STR_SIZE"),
            Result("OFF_STR_CAPACITY")
        };
    }

    std::vector<Result> ScanAll();
    void PrintResults() const;
    void ExportToFile(const std::string& filename) const;

private:
    // Main scan functions
    void ScanMapOffsets();
    void ScanEntryOffsets();
    void ScanStringOffsets();
    
    // Structure finders
    std::vector<uintptr_t> FindMapStructures();
    std::vector<uintptr_t> FindEntryStructures(const std::vector<uintptr_t>& maps);
    std::vector<uintptr_t> FindStringStructures();
    
    // Analysis functions
    std::map<int, uint64_t> ReadFields(uintptr_t addr, int maxBytes) const;
    
    // Validation
    bool IsValidPointer(uint64_t value) const;
    bool IsHeapMemory(uint64_t value) const;
    bool IsCodePointer(uint64_t value) const;
    bool IsBitMask(uint64_t value) const;
    bool IsLikelyStringData(uint64_t value) const;
    
    // Helpers
    int MostVoted(const std::map<int, int>& votes) const;
    double CalcConfidence(const std::map<int, int>& votes, int offset) const;
    uintptr_t GetStringLen(uintptr_t strAddr) const;
    std::vector<uintptr_t> GetAllocatorStructures() const;
};