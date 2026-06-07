#pragma once
#include "utils.h"
#include <Windows.h>
#include <functional>
#include <string>
#include <vector>

class MemoryScanner {
private:
    HANDLE hProcess;
    DWORD processId;
    uintptr_t baseAddress;
    uintptr_t moduleSize;

public:
    MemoryScanner() : hProcess(nullptr), processId(0), baseAddress(0), moduleSize(0) {}
    ~MemoryScanner();

    MemoryScanner(const MemoryScanner&) = delete;
    MemoryScanner& operator=(const MemoryScanner&) = delete;

    bool Initialize();
    bool IsReady() const { return hProcess != nullptr; }
    void Close();

    HANDLE GetHandle() const { return hProcess; }
    DWORD GetPid() const { return processId; }
    uintptr_t GetBase() const { return baseAddress; }
    uintptr_t GetSize() const { return moduleSize; }

    template<typename T>
    T Read(uintptr_t address) const {
        T buffer{};
        SIZE_T bytesRead = 0;
        if (hProcess && ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), &buffer, sizeof(T), &bytesRead) && bytesRead == sizeof(T)) {
            return buffer;
        }
        return T{};
    }

    std::vector<uint8_t> ReadBytes(uintptr_t address, size_t size) const;
    std::string ReadString(uintptr_t address, size_t maxLen = 256) const;

    void ScanMemory(std::function<bool(uintptr_t, const std::vector<uint8_t>&)> callback,
                    size_t chunkSize = 0x100000) const;

private:
    bool FindRobloxProcess();
    bool GetModuleInfo();
    bool IsReadableRegion(const MEMORY_BASIC_INFORMATION& mbi) const;
};
