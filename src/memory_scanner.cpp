#include "memory_scanner.h"

bool MemoryScanner::Initialize() {
    if (!FindRobloxProcess()) {
        Utils::LogError("RobloxPlayerBeta.exe not found! Please start Roblox first.");
        return false;
    }

    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (!hProcess) {
        Utils::LogError("Failed to open process. Run as Administrator!");
        return false;
    }

    if (!GetModuleInfo()) {
        Utils::LogError("Failed to get module info.");
        return false;
    }

    Utils::LogSuccess("Connected to Roblox (PID: " + std::to_string(processId) + ")");
    Utils::LogInfo("Base: " + Utils::ToHex(baseAddress) + " Size: " + 
                   std::to_string(moduleSize / 1024 / 1024) + " MB");
    return true;
}

bool MemoryScanner::FindRobloxProcess() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe32 = { sizeof(PROCESSENTRY32W) };
    
    if (Process32FirstW(snapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, L"RobloxPlayerBeta.exe") == 0) {
                processId = pe32.th32ProcessID;
                CloseHandle(snapshot);
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
    
    if (!EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) return false;

    for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
        wchar_t szModName[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, hMods[i], szModName, MAX_PATH)) {
            if (wcsstr(szModName, L"RobloxPlayerBeta.exe")) {
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

std::vector<uint8_t> MemoryScanner::ReadBytes(uintptr_t address, size_t size) const {
    std::vector<uint8_t> buffer(size);
    SIZE_T bytesRead;
    if (ReadProcessMemory(hProcess, (LPCVOID)address, buffer.data(), size, &bytesRead)) {
        buffer.resize(bytesRead);
        return buffer;
    }
    return {};
}

std::string MemoryScanner::ReadString(uintptr_t address, size_t maxLen) const {
    std::vector<char> buffer(maxLen + 1, 0);
    SIZE_T bytesRead;
    if (ReadProcessMemory(hProcess, (LPCVOID)address, buffer.data(), maxLen, &bytesRead)) {
        return std::string(buffer.data());
    }
    return "";
}

void MemoryScanner::ScanMemory(std::function<bool(uintptr_t, const std::vector<uint8_t>&)> callback, 
                                size_t chunkSize) const {
    uintptr_t addr = 0;
    while (addr < 0x7FFFFFFFFFFF) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE) {
                uintptr_t regionStart = (uintptr_t)mbi.BaseAddress;
                size_t regionSize = mbi.RegionSize;
                
                for (size_t offset = 0; offset < regionSize; offset += chunkSize) {
                    size_t readSize = std::min(chunkSize, regionSize - offset);
                    auto data = ReadBytes(regionStart + offset, readSize);
                    if (!data.empty()) {
                        if (callback(regionStart + offset, data)) return;
                    }
                }
            }
            addr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
        } else {
            addr += 0x10000;
        }
    }
}