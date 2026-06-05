#pragma once
#include "scanner.hpp"
#include "pattern_db.hpp"
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

class DynamicOffsetScanner {
private:
    MemoryScanner& scanner;
    std::vector<MemoryScanner::ScanResult> results;
    std::atomic<bool> running;
    std::atomic<float> progress;
    std::mutex resultMutex;

    // Dynamic scanning strategies
    struct ScanStrategy {
        std::string name;
        std::function<bool(const std::string&, MemoryScanner::ScanResult&)> function;
        int priority; // 0 = highest
    };

    std::vector<ScanStrategy> strategies;

public:
    DynamicOffsetScanner(MemoryScanner& memScanner);
    ~DynamicOffsetScanner();

    // Main scanning functions
    bool ScanAll();
    void ScanAsync(std::function<void(float)> progressCallback = nullptr);
    bool IsRunning() const { return running; }
    float GetProgress() const { return progress; }
    
    // Individual offset scanning
    MemoryScanner::ScanResult ScanOffset(const std::string& name);
    std::vector<MemoryScanner::ScanResult> GetAllResults() const;
    
    // Export results
    void ExportToFile(const std::string& filename) const;
    void ExportToHeader(const std::string& filename) const;
    void PrintResults() const;

    // Validation
    bool ValidateOffset(const std::string& name, uintptr_t offset) const;
    double CalculateConfidence(const std::string& name, uintptr_t offset) const;

private:
    void InitializeStrategies();
    void InitializeResults();
    
    // Strategy functions
    bool PatternScanStrategy(const std::string& name, MemoryScanner::ScanResult& result);
    bool SignatureScanStrategy(const std::string& name, MemoryScanner::ScanResult& result);
    bool StructureAnalysisStrategy(const std::string& name, MemoryScanner::ScanResult& result);
    bool BruteForceStrategy(const std::string& name, MemoryScanner::ScanResult& result);
    bool KnownOffsetsStrategy(const std::string& name, MemoryScanner::ScanResult& result);
    bool CrossReferenceStrategy(const std::string& name, MemoryScanner::ScanResult& result);
    
    // Analysis functions
    uintptr_t ParseOffsetFromInstruction(uintptr_t address) const;
    bool AnalyzeMapStructure(uintptr_t address, std::map<std::string, uintptr_t>& offsets);
    bool AnalyzeEntryStructure(uintptr_t address, std::map<std::string, uintptr_t>& offsets);
    bool AnalyzeStringStructure(uintptr_t address, std::map<std::string, uintptr_t>& offsets);
    
    // Helper functions
    std::vector<uintptr_t> FindXRefs(uintptr_t address) const;
    uintptr_t FindFunctionByString(const std::string& str) const;
};