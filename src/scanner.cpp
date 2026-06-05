#include "scanner.hpp"
#include <algorithm>
#include <sstream>

MemoryScanner::MemoryScanner() : hProcess(NULL), baseAddress(0), moduleSize(0), processId(0) {}

MemoryScanner::~MemoryScanner() {
    if (hProcess) {
        CloseHandle(hProcess);
    }
}

bool MemoryScanner::Initialize(const std::wstring& processName) {
    if (!FindProcess(processName)) {
        Utils::LogError("Failed to find process: RobloxPlayerBeta.exe");
        return false;
    }

    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (!hProcess) {
        Utils::LogError("Failed to open process. Error: " + std::to_string(GetLastError()));
        return false;
    }

    if (!GetModuleInfo()) {
        Utils::LogError("Failed to get module information");
        return false;
    }

    Utils::LogSuccess("Process opened successfully");
    Utils::LogInfo("Base Address: " + Utils::ToHex(baseAddress));
    Utils::LogInfo("Module Size: " + std::to_string(moduleSize / 1024 / 1024) + " MB");
    
    return true;
}

template<typename T>
T MemoryScanner::Read(uintptr_t address) const {
    T buffer{};
    SIZE_T bytesRead;
    if (ReadProcessMemory(hProcess, (LPCVOID)address, &buffer, sizeof(T), &bytesRead) && bytesRead == sizeof(T)) {
        return buffer;
    }
    return T{};
}

// Explicit template instantiation for common types
template uint8_t MemoryScanner::Read<uint8_t>(uintptr_t) const;
template uint16_t MemoryScanner::Read<uint16_t>(uintptr_t) const;
template uint32_t MemoryScanner::Read<uint32_t>(uintptr_t) const;
template uint64_t MemoryScanner::Read<uint64_t>(uintptr_t) const;
template int32_t MemoryScanner::Read<int32_t>(uintptr_t) const;
template int64_t MemoryScanner::Read<int64_t>(uintptr_t) const;

std::vector<uint8_t> MemoryScanner::ReadBytes(uintptr_t address, size_t size) const {
    std::vector<uint8_t> buffer(size);
    SIZE_T bytesRead;
    if (ReadProcessMemory(hProcess, (LPCVOID)address, buffer.data(), size, &bytesRead)) {
        buffer.resize(bytesRead);
        return buffer;
    }
    return {};
}

std::string MemoryScanner::ReadString(uintptr_t address, size_t maxLength) const {
    std::vector<char> buffer(maxLength + 1, 0);
    SIZE_T bytesRead;
    if (ReadProcessMemory(hProcess, (LPCVOID)address, buffer.data(), maxLength, &bytesRead)) {
        return std::string(buffer.data());
    }
    return "";
}

uintptr_t MemoryScanner::FindPattern(const std::vector<uint8_t>& pattern, const std::string& mask) const {
    if (pattern.empty() || pattern.size() != mask.size()) {
        return 0;
    }

    // Read module in chunks for better performance
    const size_t CHUNK_SIZE = 0x100000; // 1MB chunks
    std::vector<uint8_t> chunk(CHUNK_SIZE);

    for (uintptr_t i = 0; i < moduleSize; i += CHUNK_SIZE - pattern.size()) {
        size_t currentChunkSize = std::min(CHUNK_SIZE, moduleSize - i);
        uintptr_t currentAddress = baseAddress + i;

        SIZE_T bytesRead;
        if (ReadProcessMemory(hProcess, (LPCVOID)currentAddress, chunk.data(), currentChunkSize, &bytesRead)) {
            for (size_t j = 0; j < bytesRead - pattern.size(); j++) {
                bool found = true;
                for (size_t k = 0; k < pattern.size(); k++) {
                    if (mask[k] == 'x' && chunk[j + k] != pattern[k]) {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    return currentAddress + j;
                }
            }
        }
    }

    return 0;
}

std::vector<uintptr_t> MemoryScanner::FindAllPatterns(const std::vector<uint8_t>& pattern, const std::string& mask) const {
    std::vector<uintptr_t> results;
    
    if (pattern.empty() || pattern.size() != mask.size()) {
        return results;
    }

    const size_t CHUNK_SIZE = 0x100000;
    std::vector<uint8_t> chunk(CHUNK_SIZE);

    for (uintptr_t i = 0; i < moduleSize; i += CHUNK_SIZE - pattern.size()) {
        size_t currentChunkSize = std::min(CHUNK_SIZE, moduleSize - i);
        uintptr_t currentAddress = baseAddress + i;

        SIZE_T bytesRead;
        if (ReadProcessMemory(hProcess, (LPCVOID)currentAddress, chunk.data(), currentChunkSize, &bytesRead)) {
            for (size_t j = 0; j < bytesRead - pattern.size(); j++) {
                bool found = true;
                for (size_t k = 0; k < pattern.size(); k++) {
                    if (mask[k] == 'x' && chunk[j + k] != pattern[k]) {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    results.push_back(currentAddress + j);
                    j += pattern.size() - 1; // Skip ahead to avoid overlapping
                }
            }
        }
    }

    return results;
}

uintptr_t MemoryScanner::FindSignature(const std::string& signature) const {
    std::string mask;
    auto pattern = Utils::ParsePattern(signature, mask);
    return FindPattern(pattern, mask);
}

bool MemoryScanner::FindProcess(const std::wstring& processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(snapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                processId = pe32.th32ProcessID;
                CloseHandle(snapshot);
                Utils::LogSuccess("Found process: RobloxPlayerBeta.exe (PID: " + std::to_string(processId) + ")");
                return true;
            }
        } while (Process32NextW(snapshot, &pe32));
    }

    CloseHandle(snapshot);
    return false;
}

bool MemoryScanner::GetModuleInfo() {
    HMODULE hMods[1024];
    DWORD cbNeeded;
    
    if (!EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        return false;
    }

    for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
        wchar_t szModName[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(wchar_t))) {
            std::wstring modName(szModName);
            if (modName.find(L"RobloxPlayerBeta.exe") != std::wstring::npos) {
                MODULEINFO modInfo;
                if (GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo))) {
                    baseAddress = (uintptr_t)modInfo.lpBaseOfDll;
                    moduleSize = modInfo.SizeOfImage;
                    return true;
                }
            }
        }
    }

    return false;
}