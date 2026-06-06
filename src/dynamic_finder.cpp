#include "dynamic_finder.h"
#include <fstream>

std::vector<DynamicFinder::Result> DynamicFinder::ScanAll() {
    Utils::LogInfo("Starting dynamic scan...\n");
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ScanMapOffsets();
    ScanEntryOffsets();
    ScanStringOffsets();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    Utils::LogInfo("Scan completed in " + std::to_string(duration.count()) + "ms\n");
    
    return results;
}

// ==================== MAP OFFSETS ====================

void DynamicFinder::ScanMapOffsets() {
    Utils::LogInfo("Scanning MAP offsets...");
    
    auto maps = FindMapStructures();
    if (maps.empty()) {
        Utils::LogWarning("No map structures found");
        return;
    }
    
    Utils::LogInfo("Found " + std::to_string(maps.size()) + " map structures");
    
    std::map<int, int> listVotes;   // OFF_MAP_LIST
    std::map<int, int> endVotes;    // OFF_MAP_END
    std::map<int, int> maskVotes;   // OFF_MAP_MASK
    
    for (auto mapAddr : maps) {
        auto fields = ReadFields(mapAddr, 64);
        
        for (auto& [offset, value] : fields) {
            // Tìm pointer fields -> candidates cho MAP_LIST và MAP_END
            if (IsValidPointer(value) && IsHeapMemory(value)) {
                // Check if it points to tree node (MAP_LIST)
                auto nodeLeft = scanner.Read<uint64_t>(value);
                auto nodeRight = scanner.Read<uint64_t>(value + 8);
                
                if (IsHeapMemory(nodeLeft) || IsHeapMemory(nodeRight)) {
                    // Đây là node pointer
                    if (!IsHeapMemory(nodeLeft) && !IsHeapMemory(nodeRight)) {
                        // Sentinel/end node (null children)
                        endVotes[offset]++;
                    } else {
                        // Regular node (MAP_LIST)
                        listVotes[offset]++;
                    }
                }
            }
            
            // Tìm mask (power of 2 - 1)
            if (IsBitMask(value) && value > 0x0F && value < 0x1000) {
                maskVotes[offset]++;
            }
        }
    }
    
    // Set results
    if (!listVotes.empty()) {
        results[1].offset = MostVoted(listVotes);
        results[1].confidence = CalcConfidence(listVotes, results[1].offset);
        results[1].found = true;
        Utils::LogSuccess("OFF_MAP_LIST = " + Utils::ToHex(results[1].offset) + 
                         " (" + std::to_string((int)(results[1].confidence * 100)) + "%)");
    }
    
    if (!endVotes.empty()) {
        results[0].offset = MostVoted(endVotes);
        results[0].confidence = CalcConfidence(endVotes, results[0].offset);
        results[0].found = true;
        Utils::LogSuccess("OFF_MAP_END = " + Utils::ToHex(results[0].offset) + 
                         " (" + std::to_string((int)(results[0].confidence * 100)) + "%)");
    }
    
    if (!maskVotes.empty()) {
        results[2].offset = MostVoted(maskVotes);
        results[2].confidence = CalcConfidence(maskVotes, results[2].offset);
        results[2].found = true;
        Utils::LogSuccess("OFF_MAP_MASK = " + Utils::ToHex(results[2].offset) + 
                         " (" + std::to_string((int)(results[2].confidence * 100)) + "%)");
    }
}

// ==================== ENTRY OFFSETS ====================

void DynamicFinder::ScanEntryOffsets() {
    Utils::LogInfo("Scanning ENTRY offsets...");
    
    auto maps = FindMapStructures();
    auto entries = FindEntryStructures(maps);
    
    if (entries.empty()) {
        Utils::LogWarning("No entry structures found");
        return;
    }
    
    Utils::LogInfo("Found " + std::to_string(entries.size()) + " entry structures");
    
    std::map<int, int> forwardVotes;  // OFF_ENTRY_FORWARD
    std::map<int, int> stringVotes;   // OFF_ENTRY_STRING
    std::map<int, int> getsetVotes;   // OFF_ENTRY_GETSET
    
    for (size_t i = 0; i < entries.size(); i++) {
        auto fields = ReadFields(entries[i], 64);
        
        for (auto& [offset, value] : fields) {
            // FORWARD: pointer to another entry
            if (IsValidPointer(value) && IsHeapMemory(value)) {
                // Check if it points to another entry
                for (size_t j = 0; j < entries.size(); j++) {
                    if (i != j && value == entries[j]) {
                        forwardVotes[offset]++;
                        break;
                    }
                }
            }
            
            // STRING: pointer to string data or inline string
            if (IsLikelyStringData(value)) {
                stringVotes[offset]++;
            }
            
            // GETSET: 2 function pointers liên tiếp
            if (IsCodePointer(value)) {
                uint64_t nextValue = 0;
                if (offset + 8 < 64) {
                    auto nextFields = ReadFields(entries[i] + offset + 8, 8);
                    if (!nextFields.empty()) {
                        nextValue = nextFields.begin()->second;
                    }
                }
                if (IsCodePointer(nextValue)) {
                    getsetVotes[offset]++;
                }
            }
        }
    }
    
    // Set results
    if (!forwardVotes.empty()) {
        results[3].offset = MostVoted(forwardVotes);
        results[3].confidence = CalcConfidence(forwardVotes, results[3].offset);
        results[3].found = true;
        Utils::LogSuccess("OFF_ENTRY_FORWARD = " + Utils::ToHex(results[3].offset) + 
                         " (" + std::to_string((int)(results[3].confidence * 100)) + "%)");
    }
    
    if (!stringVotes.empty()) {
        results[4].offset = MostVoted(stringVotes);
        results[4].confidence = CalcConfidence(stringVotes, results[4].offset);
        results[4].found = true;
        Utils::LogSuccess("OFF_ENTRY_STRING = " + Utils::ToHex(results[4].offset) + 
                         " (" + std::to_string((int)(results[4].confidence * 100)) + "%)");
    }
    
    if (!getsetVotes.empty()) {
        results[5].offset = MostVoted(getsetVotes);
        results[5].confidence = CalcConfidence(getsetVotes, results[5].offset);
        results[5].found = true;
        Utils::LogSuccess("OFF_ENTRY_GETSET = " + Utils::ToHex(results[5].offset) + 
                         " (" + std::to_string((int)(results[5].confidence * 100)) + "%)");
    }
}

// ==================== STRING OFFSETS ====================

void DynamicFinder::ScanStringOffsets() {
    Utils::LogInfo("Scanning STRING offsets...");
    
    auto strings = FindStringStructures();
    if (strings.empty()) {
        Utils::LogWarning("No string structures found");
        return;
    }
    
    Utils::LogInfo("Found " + std::to_string(strings.size()) + " string structures");
    
    std::map<int, int> sizeVotes;      // OFF_STR_SIZE
    std::map<int, int> capacityVotes;  // OFF_STR_CAPACITY
    
    for (auto strAddr : strings) {
        auto fields = ReadFields(strAddr, 32);
        uintptr_t strLen = GetStringLen(strAddr);
        
        for (auto& [offset, value] : fields) {
            // STR_SIZE: value == string length
            if (value == strLen && strLen > 0 && strLen < 10000) {
                sizeVotes[offset]++;
            }
            
            // STR_CAPACITY: value >= string length, thường > size
            if (value >= strLen && value < 100000 && value != strLen) {
                capacityVotes[offset]++;
            }
        }
    }
    
    // Set results
    if (!sizeVotes.empty()) {
        results[6].offset = MostVoted(sizeVotes);
        results[6].confidence = CalcConfidence(sizeVotes, results[6].offset);
        results[6].found = true;
        Utils::LogSuccess("OFF_STR_SIZE = " + Utils::ToHex(results[6].offset) + 
                         " (" + std::to_string((int)(results[6].confidence * 100)) + "%)");
    }
    
    if (!capacityVotes.empty()) {
        results[7].offset = MostVoted(capacityVotes);
        results[7].confidence = CalcConfidence(capacityVotes, results[7].offset);
        results[7].found = true;
        Utils::LogSuccess("OFF_STR_CAPACITY = " + Utils::ToHex(results[7].offset) + 
                         " (" + std::to_string((int)(results[7].confidence * 100)) + "%)");
    }
}

// ==================== STRUCTURE FINDERS ====================

std::vector<uintptr_t> DynamicFinder::FindMapStructures() {
    std::vector<uintptr_t> maps;
    
    scanner.ScanMemory([&](uintptr_t addr, const std::vector<uint8_t>& data) -> bool {
        for (size_t i = 0; i + 32 < data.size(); i += 8) {
            uintptr_t structAddr = addr + i;
            auto fields = ReadFields(structAddr, 32);
            
            int ptrCount = 0, intCount = 0;
            for (auto& [off, val] : fields) {
                if (IsValidPointer(val)) ptrCount++;
                else if (val > 0 && val < 0x1000000) intCount++;
            }
            
            // Map structure: 2-4 pointers + 1-2 integers
            if (ptrCount >= 2 && ptrCount <= 4 && intCount >= 1) {
                maps.push_back(structAddr);
                if (maps.size() >= 50) return true; // Đủ sample
            }
        }
        return false;
    });
    
    return maps;
}

std::vector<uintptr_t> DynamicFinder::FindEntryStructures(const std::vector<uintptr_t>& maps) {
    std::vector<uintptr_t> entries;
    std::set<uintptr_t> visited;
    
    for (auto mapAddr : maps) {
        auto fields = ReadFields(mapAddr, 64);
        
        for (auto& [offset, value] : fields) {
            if (!IsValidPointer(value) || !IsHeapMemory(value)) continue;
            if (visited.count(value)) continue;
            
            // Check if this looks like tree root
            auto left = scanner.Read<uint64_t>(value);
            auto right = scanner.Read<uint64_t>(value + 8);
            
            if (IsHeapMemory(left) && IsHeapMemory(right)) {
                // Traverse tree
                std::function<void(uintptr_t)> traverse = [&](uintptr_t node) {
                    if (!node || visited.count(node)) return;
                    visited.insert(node);
                    
                    auto l = scanner.Read<uint64_t>(node);
                    auto r = scanner.Read<uint64_t>(node + 8);
                    auto p = scanner.Read<uint64_t>(node + 16);
                    
                    // Entry data starts after tree pointers (24 bytes)
                    uintptr_t entryData = node + 24;
                    entries.push_back(entryData);
                    
                    if (l && l != node) traverse(l);
                    if (r && r != node) traverse(r);
                };
                
                traverse(value);
            }
        }
    }
    
    return entries;
}

std::vector<uintptr_t> DynamicFinder::FindStringStructures() {
    std::vector<uintptr_t> strings;
    
    scanner.ScanMemory([&](uintptr_t addr, const std::vector<uint8_t>& data) -> bool {
        for (size_t i = 0; i + 24 < data.size(); i += 8) {
            uintptr_t structAddr = addr + i;
            
            // String: [data_ptr(8)] [size(8)] [capacity(8)]
            uint64_t dataPtr = *(uint64_t*)&data[i];
            uint64_t size = *(uint64_t*)&data[i + 8];
            uint64_t cap = *(uint64_t*)&data[i + 16];
            
            if (IsHeapMemory(dataPtr) && size > 0 && size < 10000 && cap >= size) {
                // Verify it's actually string data
                auto strData = scanner.ReadBytes(dataPtr, std::min((size_t)size, (size_t)50));
                if (!strData.empty()) {
                    bool isPrintable = true;
                    for (size_t j = 0; j < strData.size() && j < 20; j++) {
                        if (strData[j] != 0 && (strData[j] < 32 || strData[j] > 126)) {
                            isPrintable = false;
                            break;
                        }
                    }
                    if (isPrintable) {
                        strings.push_back(structAddr);
                        if (strings.size() >= 30) return true;
                    }
                }
            }
        }
        return false;
    });
    
    return strings;
}

// ==================== HELPERS ====================

std::map<int, uint64_t> DynamicFinder::ReadFields(uintptr_t addr, int maxBytes) {
    std::map<int, uint64_t> fields;
    auto data = scanner.ReadBytes(addr, maxBytes);
    
    for (size_t off = 0; off + 8 <= data.size(); off += 8) {
        uint64_t val = *(uint64_t*)&data[off];
        if (val != 0) {
            fields[off] = val;
        }
    }
    return fields;
}

bool DynamicFinder::IsValidPointer(uint64_t value) const {
    return value > 0x10000 && value < 0x7FFFFFFFFFFF;
}

bool DynamicFinder::IsHeapMemory(uint64_t value) const {
    if (!IsValidPointer(value)) return false;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(scanner.GetHandle(), (LPCVOID)value, &mbi, sizeof(mbi))) {
        return mbi.Type == MEM_PRIVATE && mbi.State == MEM_COMMIT;
    }
    return false;
}

bool DynamicFinder::IsCodePointer(uint64_t value) const {
    return value >= scanner.GetBase() && value < scanner.GetBase() + scanner.GetSize();
}

bool DynamicFinder::IsBitMask(uint64_t value) const {
    return value > 0 && ((value + 1) & value) == 0;
}

bool DynamicFinder::IsLikelyStringData(uint64_t value) const {
    if (IsHeapMemory(value)) {
        auto data = scanner.ReadBytes(value, 32);
        int printable = 0;
        for (size_t i = 0; i < data.size() && data[i] != 0; i++) {
            if (data[i] >= 32 && data[i] <= 126) printable++;
        }
        return printable > 3;
    }
    
    // Check inline string
    char* chars = (char*)&value;
    int len = 0;
    for (int i = 0; i < 8 && chars[i] != 0; i++) {
        if (chars[i] >= 32 && chars[i] <= 126) len++;
    }
    return len >= 2;
}

int DynamicFinder::MostVoted(const std::map<int, int>& votes) const {
    int best = 0, bestOff = 0;
    for (auto& [off, cnt] : votes) {
        if (cnt > best) { best = cnt; bestOff = off; }
    }
    return bestOff;
}

double DynamicFinder::CalcConfidence(const std::map<int, int>& votes, int offset) const {
    if (votes.empty()) return 0.0;
    int total = 0, max = 0;
    for (auto& [off, cnt] : votes) {
        total += cnt;
        if (off == offset) max = cnt;
    }
    return total > 0 ? (double)max / total : 0.0;
}

uintptr_t DynamicFinder::GetStringLen(uintptr_t strAddr) const {
    auto fields = ReadFields(strAddr, 32);
    
    // Find data pointer and read string
    for (auto& [off, val] : fields) {
        if (IsHeapMemory(val)) {
            auto str = scanner.ReadString(val, 1024);
            return str.length();
        }
    }
    return 0;
}

void DynamicFinder::PrintResults() const {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "                    SCAN RESULTS\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    int found = 0;
    for (auto& r : results) {
        std::cout << std::setw(25) << std::left << r.name << " : ";
        if (r.found) {
            Utils::SetColor(Utils::Color::GREEN);
            std::cout << Utils::ToHex(r.offset) << " (" 
                     << (int)(r.confidence * 100) << "%)";
            Utils::ResetColor();
            found++;
        } else {
            Utils::SetColor(Utils::Color::RED);
            std::cout << "NOT FOUND";
            Utils::ResetColor();
        }
        std::cout << "\n";
    }
    
    std::cout << "\n" << std::string(70, '-') << "\n";
    std::cout << "Found: " << found << "/" << results.size() << " offsets\n";
    std::cout << std::string(70, '=') << "\n\n";
}

void DynamicFinder::ExportToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        Utils::LogError("Cannot write to " + filename);
        return;
    }
    
    file << "// Roblox Offsets - Auto-generated\n";
    file << "// " << __DATE__ << " " << __TIME__ << "\n\n";
    
    for (auto& r : results) {
        if (r.found) {
            file << "#define " << r.name << " 0x" << std::hex << r.offset << std::dec << "\n";
        } else {
            file << "// #define " << r.name << " NOT_FOUND\n";
        }
    }
    
    file.close();
    Utils::LogSuccess("Exported to: " + filename);
}