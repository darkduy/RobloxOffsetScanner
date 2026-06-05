#include "dynamic_scanner.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <map>
#include <set>

DynamicOffsetScanner::DynamicOffsetScanner(MemoryScanner& memScanner) 
    : scanner(memScanner), running(false), progress(0.0f) {
    InitializeResults();
    InitializeStrategies();
}

DynamicOffsetScanner::~DynamicOffsetScanner() {
    running = false;
    if (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void DynamicOffsetScanner::InitializeResults() {
    results.clear();
    std::vector<std::string> offsetNames = {
        "OFF_MAP_END", "OFF_MAP_LIST", "OFF_MAP_MASK",
        "OFF_ENTRY_FORWARD", "OFF_ENTRY_STRING", "OFF_ENTRY_GETSET",
        "OFF_STR_SIZE", "OFF_STR_CAPACITY"
    };
    
    for (const auto& name : offsetNames) {
        results.push_back(MemoryScanner::ScanResult(name));
    }
}

void DynamicOffsetScanner::InitializeStrategies() {
    strategies = {
        {"Known Offsets", [this](const std::string& name, MemoryScanner::ScanResult& result) {
            return KnownOffsetsStrategy(name, result);
        }, 0},
        
        {"Pattern Scan", [this](const std::string& name, MemoryScanner::ScanResult& result) {
            return PatternScanStrategy(name, result);
        }, 1},
        
        {"Signature Scan", [this](const std::string& name, MemoryScanner::ScanResult& result) {
            return SignatureScanStrategy(name, result);
        }, 2},
        
        {"Structure Analysis", [this](const std::string& name, MemoryScanner::ScanResult& result) {
            return StructureAnalysisStrategy(name, result);
        }, 3},
        
        {"Cross Reference", [this](const std::string& name, MemoryScanner::ScanResult& result) {
            return CrossReferenceStrategy(name, result);
        }, 4},
        
        {"Brute Force", [this](const std::string& name, MemoryScanner::ScanResult& result) {
            return BruteForceStrategy(name, result);
        }, 5}
    };
    
    // Sort by priority
    std::sort(strategies.begin(), strategies.end(), 
        [](const ScanStrategy& a, const ScanStrategy& b) {
            return a.priority < b.priority;
        });
}

bool DynamicOffsetScanner::ScanAll() {
    running = true;
    progress = 0.0f;
    
    Utils::LogInfo("Starting dynamic offset scan...");
    std::cout << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Scan each offset
    for (size_t i = 0; i < results.size(); i++) {
        if (!running) break;
        
        auto& result = results[i];
        Utils::LogInfo("Scanning " + result.name + "...");
        
        // Try each strategy
        bool found = false;
        for (const auto& strategy : strategies) {
            if (found) break;
            
            MemoryScanner::ScanResult temp(result.name);
            if (strategy.function(result.name, temp)) {
                if (ValidateOffset(result.name, temp.offset)) {
                    result = temp;
                    result.found = true;
                    result.confidence = CalculateConfidence(result.name, result.offset);
                    found = true;
                    
                    Utils::LogSuccess(result.name + " = " + Utils::ToHex(result.offset) + 
                        " (confidence: " + std::to_string((int)(result.confidence * 100)) + "%)" +
                        " [" + result.method + "]");
                }
            }
        }
        
        if (!found) {
            Utils::LogWarning(result.name + " not found with high confidence");
        }
        
        progress = (float)(i + 1) / results.size();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    running = false;
    Utils::LogInfo("Scan completed in " + std::to_string(duration.count()) + "ms");
    
    return true;
}

void DynamicOffsetScanner::ScanAsync(std::function<void(float)> progressCallback) {
    std::thread([this, progressCallback]() {
        ScanAll();
        if (progressCallback) {
            progressCallback(progress);
        }
    }).detach();
}

MemoryScanner::ScanResult DynamicOffsetScanner::ScanOffset(const std::string& name) {
    for (const auto& result : results) {
        if (result.name == name) {
            return result;
        }
    }
    return MemoryScanner::ScanResult(name);
}

std::vector<MemoryScanner::ScanResult> DynamicOffsetScanner::GetAllResults() const {
    return results;
}

bool DynamicOffsetScanner::PatternScanStrategy(const std::string& name, MemoryScanner::ScanResult& result) {
    auto patterns = PatternDB::GetPatterns();
    
    for (const auto& pattern : patterns) {
        if (pattern.name != name) continue;
        
        for (const auto& sig : pattern.patterns) {
            uintptr_t addr = scanner.FindSignature(sig);
            if (addr) {
                uintptr_t offset = ParseOffsetFromInstruction(addr);
                if (offset > 0 && offset < 0x200) {
                    result.offset = offset;
                    result.method = "Pattern Scan";
                    result.confidence = 0.8;
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool DynamicOffsetScanner::SignatureScanStrategy(const std::string& name, MemoryScanner::ScanResult& result) {
    // Advanced signature scanning with multiple variants
    std::map<std::string, std::vector<std::string>> signatureMap = {
        {"OFF_MAP_END", {
            "48 8B 47 ?? 48 89 45",
            "48 8B 87 ?? ?? ?? ?? 48 89",
            "8B 47 ?? 48 89 45",
            "48 8B 47 ?? 48 85 C0 74"
        }},
        {"OFF_MAP_LIST", {
            "48 8D 4F ?? 48 8B D7",
            "48 8B 47 ?? 48 8B CF",
            "48 8D 47 ?? 48 89 45"
        }},
        {"OFF_MAP_MASK", {
            "83 E0 ?? 48 8B 4C",
            "25 ?? ?? ?? ?? 48 8B",
            "83 E1 ?? 48 8B"
        }},
        {"OFF_ENTRY_FORWARD", {
            "48 8B 41 ?? 48 85 C0 74",
            "48 8B 47 ?? 48 85 C0 74",
            "48 8B 41 ?? 48 89 45"
        }},
        {"OFF_ENTRY_STRING", {
            "48 8B 41 ?? 48 8B 50",
            "48 8B 47 ?? 48 8B 48",
            "48 8B 47 ?? 48 8B 4F"
        }},
        {"OFF_ENTRY_GETSET", {
            "48 8B 41 ?? 48 85 C0 0F",
            "48 8B 47 ?? 48 85 C0 0F",
            "4C 8B 41 ?? 4D 85 C0"
        }},
        {"OFF_STR_SIZE", {
            "83 78 ?? 00 7C",
            "83 79 ?? 00 7C",
            "8B 47 ?? 85 C0 7E"
        }},
        {"OFF_STR_CAPACITY", {
            "3B 47 ?? 7C",
            "3B 41 ?? 7C",
            "8B 47 ?? 3B 47"
        }}
    };
    
    auto it = signatureMap.find(name);
    if (it != signatureMap.end()) {
        for (const auto& sig : it->second) {
            uintptr_t addr = scanner.FindSignature(sig);
            if (addr) {
                uintptr_t offset = ParseOffsetFromInstruction(addr);
                if (offset > 0 && offset < 0x200) {
                    result.offset = offset;
                    result.method = "Signature Scan";
                    result.confidence = 0.85;
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool DynamicOffsetScanner::StructureAnalysisStrategy(const std::string& name, MemoryScanner::ScanResult& result) {
    // Find and analyze structure references
    if (name.find("MAP_") != std::string::npos) {
        // Find map-related functions
        uintptr_t mapFunc = scanner.FindSignature("48 89 5C 24 ?? 57 48 83 EC 20 48 8B D9");
        if (mapFunc) {
            std::map<std::string, uintptr_t> mapOffsets;
            if (AnalyzeMapStructure(mapFunc, mapOffsets)) {
                auto it = mapOffsets.find(name);
                if (it != mapOffsets.end()) {
                    result.offset = it->second;
                    result.method = "Structure Analysis";
                    result.confidence = 0.75;
                    return true;
                }
            }
        }
    }
    else if (name.find("ENTRY_") != std::string::npos) {
        uintptr_t entryFunc = scanner.FindSignature("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20");
        if (entryFunc) {
            std::map<std::string, uintptr_t> entryOffsets;
            if (AnalyzeEntryStructure(entryFunc, entryOffsets)) {
                auto it = entryOffsets.find(name);
                if (it != entryOffsets.end()) {
                    result.offset = it->second;
                    result.method = "Structure Analysis";
                    result.confidence = 0.75;
                    return true;
                }
            }
        }
    }
    else if (name.find("STR_") != std::string::npos) {
        uintptr_t strFunc = scanner.FindSignature("48 89 5C 24 ?? 57 48 83 EC 20 48 8B D9");
        if (strFunc) {
            std::map<std::string, uintptr_t> strOffsets;
            if (AnalyzeStringStructure(strFunc, strOffsets)) {
                auto it = strOffsets.find(name);
                if (it != strOffsets.end()) {
                    result.offset = it->second;
                    result.method = "Structure Analysis";
                    result.confidence = 0.75;
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool DynamicOffsetScanner::BruteForceStrategy(const std::string& name, MemoryScanner::ScanResult& result) {
    // Try common offset ranges
    std::map<std::string, std::vector<uintptr_t>> commonOffsets = {
        {"OFF_MAP_END", {0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28}},
        {"OFF_MAP_LIST", {0x00, 0x04, 0x08, 0x0C, 0x10, 0x14}},
        {"OFF_MAP_MASK", {0x7F, 0x3F, 0x1F, 0xFF, 0x0F}},
        {"OFF_ENTRY_FORWARD", {0x00, 0x04, 0x08, 0x0C, 0x10, 0x14}},
        {"OFF_ENTRY_STRING", {0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C}},
        {"OFF_ENTRY_GETSET", {0x10, 0x14, 0x18, 0x1C, 0x20, 0x24}},
        {"OFF_STR_SIZE", {0x0C, 0x10, 0x14, 0x18, 0x1C}},
        {"OFF_STR_CAPACITY", {0x10, 0x14, 0x18, 0x1C, 0x20, 0x24}}
    };
    
    auto it = commonOffsets.find(name);
    if (it != commonOffsets.end()) {
        for (uintptr_t offset : it->second) {
            if (ValidateOffset(name, offset)) {
                double confidence = CalculateConfidence(name, offset);
                if (confidence > 0.6) {
                    result.offset = offset;
                    result.method = "Brute Force";
                    result.confidence = confidence;
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool DynamicOffsetScanner::KnownOffsetsStrategy(const std::string& name, MemoryScanner::ScanResult& result) {
    auto knownVersions = PatternDB::GetKnownOffsets();
    
    // Try each known version's offsets
    for (const auto& version : knownVersions) {
        auto it = version.offsets.find(name);
        if (it != version.offsets.end()) {
            uintptr_t offset = it->second;
            if (ValidateOffset(name, offset)) {
                result.offset = offset;
                result.method = "Known Offsets (" + version.version + ")";
                result.confidence = 0.9;
                return true;
            }
        }
    }
    
    return false;
}

bool DynamicOffsetScanner::CrossReferenceStrategy(const std::string& name, MemoryScanner::ScanResult& result) {
    // Find related structures and cross-reference
    // This is a simplified version - in practice would need more complex analysis
    
    if (name == "OFF_MAP_END" || name == "OFF_MAP_LIST") {
        // If we found OFF_MAP_LIST, use it to find OFF_MAP_END
        for (const auto& r : results) {
            if (r.found && r.name == "OFF_MAP_LIST") {
                // MAP_END is typically MAP_LIST + 8 or similar
                uintptr_t potentialOffset = r.offset + 0x08;
                if (ValidateOffset(name, potentialOffset)) {
                    result.offset = potentialOffset;
                    result.method = "Cross Reference";
                    result.confidence = 0.7;
                    return true;
                }
            }
        }
    }
    
    if (name == "OFF_STR_CAPACITY") {
        // STR_CAPACITY is often STR_SIZE + 4 or +8
        for (const auto& r : results) {
            if (r.found && r.name == "OFF_STR_SIZE") {
                uintptr_t potentialOffset = r.offset + 0x04;
                if (ValidateOffset(name, potentialOffset)) {
                    result.offset = potentialOffset;
                    result.method = "Cross Reference";
                    result.confidence = 0.7;
                    return true;
                }
            }
        }
    }
    
    return false;
}

uintptr_t DynamicOffsetScanner::ParseOffsetFromInstruction(uintptr_t address) const {
    auto bytes = scanner.ReadBytes(address, 8);
    if (bytes.size() < 4) return 0;
    
    // Parse various instruction formats
    // mov rax/eax, [reg+offset]
    if (bytes[0] == 0x48 && bytes[1] == 0x8B && bytes[2] >= 0x40 && bytes[2] <= 0x7F) {
        if (bytes.size() >= 4) return bytes[3];
    }
    if (bytes[0] == 0x8B && bytes[1] >= 0x40 && bytes[1] <= 0x7F) {
        if (bytes.size() >= 3) return bytes[2];
    }
    
    // and eax/ecx, offset
    if (bytes[0] == 0x83 && bytes[1] == 0xE0) {
        if (bytes.size() >= 3) return bytes[2];
    }
    
    // cmp dword ptr [reg+offset], 0
    if (bytes[0] == 0x83 && (bytes[1] >= 0x78 && bytes[1] <= 0x7F)) {
        if (bytes.size() >= 3) return bytes[2];
    }
    
    // lea reg, [reg+offset]
    if (bytes[0] == 0x48 && bytes[1] == 0x8D && bytes[2] >= 0x40 && bytes[2] <= 0x7F) {
        if (bytes.size() >= 4) return bytes[3];
    }
    
    return 0;
}

bool DynamicOffsetScanner::AnalyzeMapStructure(uintptr_t address, std::map<std::string, uintptr_t>& offsets) {
    // Analyze function to find map structure offsets
    auto bytes = scanner.ReadBytes(address, 512);
    if (bytes.empty()) return false;
    
    for (size_t i = 0; i < bytes.size() - 4; i++) {
        // Look for common patterns
        if (bytes[i] == 0x48 && bytes[i+1] == 0x8B) {
            uint8_t reg = bytes[i+2];
            if (reg >= 0x40 && reg <= 0x4F) { // [r8-r15]
                uint8_t offset = bytes[i+3];
                
                if (offset >= 0x00 && offset <= 0x30) {
                    if (!offsets.count("OFF_MAP_LIST")) {
                        offsets["OFF_MAP_LIST"] = offset;
                    }
                }
            }
        }
        
        if (bytes[i] == 0x83 && bytes[i+1] == 0xE0) { // and eax
            uint8_t offset = bytes[i+2];
            if (offset == 0x7F || offset == 0x3F || offset == 0x1F) {
                offsets["OFF_MAP_MASK"] = offset;
            }
        }
    }
    
    return !offsets.empty();
}

bool DynamicOffsetScanner::AnalyzeEntryStructure(uintptr_t address, std::map<std::string, uintptr_t>& offsets) {
    auto bytes = scanner.ReadBytes(address, 512);
    if (bytes.empty()) return false;
    
    std::set<uint8_t> foundOffsets;
    
    for (size_t i = 0; i < bytes.size() - 4; i++) {
        if (bytes[i] == 0x48 && bytes[i+1] == 0x8B && bytes[i+2] >= 0x40 && bytes[i+2] <= 0x4F) {
            uint8_t offset = bytes[i+3];
            foundOffsets.insert(offset);
        }
    }
    
    // Sort and assign
    std::vector<uint8_t> sorted(foundOffsets.begin(), foundOffsets.end());
    std::sort(sorted.begin(), sorted.end());
    
    if (sorted.size() >= 3) {
        offsets["OFF_ENTRY_FORWARD"] = sorted[0];
        offsets["OFF_ENTRY_STRING"] = sorted[1];
        offsets["OFF_ENTRY_GETSET"] = sorted[2];
        return true;
    }
    
    return false;
}

bool DynamicOffsetScanner::AnalyzeStringStructure(uintptr_t address, std::map<std::string, uintptr_t>& offsets) {
    auto bytes = scanner.ReadBytes(address, 512);
    if (bytes.empty()) return false;
    
    std::set<uint8_t> foundOffsets;
    
    for (size_t i = 0; i < bytes.size() - 4; i++) {
        // cmp instructions for size/capacity
        if (bytes[i] == 0x83 && (bytes[i+1] >= 0x78 && bytes[i+1] <= 0x7F)) {
            uint8_t offset = bytes[i+2];
            if (offset >= 0x08 && offset <= 0x24) {
                foundOffsets.insert(offset);
            }
        }
    }
    
    std::vector<uint8_t> sorted(foundOffsets.begin(), foundOffsets.end());
    std::sort(sorted.begin(), sorted.end());
    
    if (sorted.size() >= 2) {
        offsets["OFF_STR_SIZE"] = sorted[0];
        offsets["OFF_STR_CAPACITY"] = sorted[1];
        return true;
    }
    
    return false;
}

bool DynamicOffsetScanner::ValidateOffset(const std::string& name, uintptr_t offset) const {
    // Basic validation
    if (offset == 0 || offset > 0x200) return false;
    
    // Specific validations
    if (name == "OFF_MAP_MASK") {
        return offset == 0x7F || offset == 0x3F || offset == 0x1F || 
               offset == 0xFF || offset == 0x0F;
    }
    
    // Offsets should be aligned to 4 or 8 bytes
    if (name != "OFF_MAP_MASK") {
        return (offset % 4 == 0) || (offset % 8 == 0);
    }
    
    return true;
}

double DynamicOffsetScanner::CalculateConfidence(const std::string& name, uintptr_t offset) const {
    double confidence = 0.5;
    
    // Increase confidence based on multiple factors
    if (ValidateOffset(name, offset)) confidence += 0.1;
    
    // Check if offset matches known patterns
    auto knownVersions = PatternDB::GetKnownOffsets();
    for (const auto& version : knownVersions) {
        auto it = version.offsets.find(name);
        if (it != version.offsets.end() && it->second == offset) {
            confidence += 0.2;
            break;
        }
    }
    
    // If found through multiple methods
    int foundCount = 0;
    for (const auto& result : results) {
        if (result.name == name && result.found) foundCount++;
    }
    confidence += std::min(0.2, foundCount * 0.1);
    
    return std::min(1.0, confidence);
}

void DynamicOffsetScanner::ExportToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        Utils::LogError("Failed to open file: " + filename);
        return;
    }
    
    file << "// Roblox Offset Scanner Results\n";
    file << "// Generated: " << __DATE__ << " " << __TIME__ << "\n\n";
    
    for (const auto& result : results) {
        if (result.found) {
            file << "#define " << result.name << " 0x" << std::hex << result.offset << std::dec;
            file << " // Confidence: " << (int)(result.confidence * 100) << "%";
            file << " Method: " << result.method;
        } else {
            file << "// #define " << result.name << " NOT_FOUND";
        }
        file << "\n";
    }
    
    file.close();
    Utils::LogSuccess("Results exported to: " + filename);
}

void DynamicOffsetScanner::ExportToHeader(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        Utils::LogError("Failed to open file: " + filename);
        return;
    }
    
    file << "#pragma once\n\n";
    file << "// Auto-generated Roblox Offsets\n";
    file << "namespace RobloxOffsets {\n\n";
    
    for (const auto& result : results) {
        if (result.found) {
            file << "    constexpr uintptr_t " << result.name << " = 0x" 
                 << std::hex << result.offset << ";\n";
        }
    }
    
    file << "\n} // namespace RobloxOffsets\n";
    
    file.close();
    Utils::LogSuccess("Header exported to: " + filename);
}

void DynamicOffsetScanner::PrintResults() const {
    std::cout << "\n";
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "                    SCAN RESULTS SUMMARY" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
    
    int found = 0;
    for (const auto& result : results) {
        std::cout << std::setw(25) << std::left << result.name << " : ";
        
        if (result.found) {
            Utils::SetColor(Utils::Color::GREEN);
            std::cout << Utils::ToHex(result.offset);
            Utils::ResetColor();
            std::cout << " [" << (int)(result.confidence * 100) << "%] ";
            std::cout << "(" << result.method << ")";
            found++;
        } else {
            Utils::SetColor(Utils::Color::RED);
            std::cout << "NOT FOUND";
            Utils::ResetColor();
        }
        std::cout << std::endl;
    }
    
    std::cout << std::string(70, '-') << std::endl;
    std::cout << "Found: " << found << "/" << results.size() << " offsets" << std::endl;
    std::cout << std::string(70, '=') << std::endl;
}