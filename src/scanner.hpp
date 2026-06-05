#pragma once
#include "utils.hpp"
#include <Windows.h>
#include <vector>
#include <string>
#include <map>
#include <functional>

class MemoryScanner {
private:
    HANDLE hProcess;
    uintptr_t baseAddress;
    uintptr_t moduleSize;
    DWORD processId;

public:
    struct ScanResult {
        std::string name;
        uintptr_t offset;
        bool found;
        double confidence; // 0.0 - 1.0
        std::string method; // How it was found

        ScanResult(const std::string& n) : name(n), offset(0), found(false), confidence(0.0), method("") {}
    };

    MemoryScanner();
    ~MemoryScanner();

    bool Initialize(const std::wstring& processName = L"RobloxPlayerBeta.exe");
    bool IsInitialized() const { return hProcess != NULL; }

    // Memory reading
    template<typename T>
    T Read(uintptr_t address) const;

    std::vector<uint8_t> ReadBytes(uintptr_t address, size_t size) const;
    std::string ReadString(uintptr_t address, size_t maxLength = 256) const;

    // Pattern scanning
    uintptr_t FindPattern(const std::vector<uint8_t>& pattern, const std::string& mask) const;
    std::vector<uintptr_t> FindAllPatterns(const std::vector<uint8_t>& pattern, const std::string& mask) const;

    // Signature scanning with wildcards
    uintptr_t FindSignature(const std::string& signature) const;

    // Getters
    uintptr_t GetBaseAddress() const { return baseAddress; }
    uintptr_t GetModuleSize() const { return moduleSize; }
    HANDLE GetProcessHandle() const { return hProcess; }
    DWORD GetProcessId() const { return processId; }

private:
    bool FindProcess(const std::wstring& processName);
    bool GetModuleInfo();
};