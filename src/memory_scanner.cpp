#include "memory_scanner.h"

namespace {
    constexpr uintptr_t kFallbackAddressStep = 0x10000;
}

MemoryScanner::~MemoryScanner() {
    Close();
}

void MemoryScanner::Close() {
    if (hProcess) {
        CloseHandle(hProcess);
        hProcess = nullptr;
    }
    processId = 0;
    baseAddress = 0;
    moduleSize = 0;
}

bool MemoryScanner::Initialize() {
    Close();

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
        Close();
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
    HMODULE hMods[1024]{};
    DWORD cbNeeded = 0;

    if (!EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) return false;

    const DWORD moduleCount = cbNeeded / sizeof(HMODULE);
    for (DWORD i = 0; i < moduleCount; i++) {
        wchar_t szModName[MAX_PATH]{};
        if (GetModuleFileNameExW(hProcess, hMods[i], szModName, MAX_PATH)) {
            if (wcsstr(szModName, L"RobloxPlayerBeta.exe")) {
                MODULEINFO modInfo{};
                if (GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo))) {
                    baseAddress = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
                    moduleSize = modInfo.SizeOfImage;
                    return true;
                }
            }
        }
    }
    return false;
}

std::vector<uint8_t> MemoryScanner::ReadBytes(uintptr_t address, size_t size) const {
    if (!hProcess || address == 0 || size == 0) {
        return {};
    }

    std::vector<uint8_t> buffer(size);
    SIZE_T bytesRead = 0;
    if (ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), buffer.data(), size, &bytesRead) && bytesRead > 0) {
        buffer.resize(bytesRead);
        return buffer;
    }
    return {};
}

std::string MemoryScanner::ReadString(uintptr_t address, size_t maxLen) const {
    auto bytes = ReadBytes(address, maxLen);
    if (bytes.empty()) {
        return "";
    }

    const auto terminator = std::find(bytes.begin(), bytes.end(), static_cast<uint8_t>(0));
    return std::string(reinterpret_cast<const char*>(bytes.data()), terminator - bytes.begin());
}

bool MemoryScanner::IsReadableRegion(const MEMORY_BASIC_INFORMATION& mbi) const {
    if (mbi.State != MEM_COMMIT || mbi.Type != MEM_PRIVATE) {
        return false;
    }

    if ((mbi.Protect & PAGE_GUARD) || (mbi.Protect & PAGE_NOACCESS)) {
        return false;
    }

    const DWORD protection = mbi.Protect & 0xFF;
    return protection == PAGE_READONLY ||
           protection == PAGE_READWRITE ||
           protection == PAGE_WRITECOPY ||
           protection == PAGE_EXECUTE_READ ||
           protection == PAGE_EXECUTE_READWRITE ||
           protection == PAGE_EXECUTE_WRITECOPY;
}

void MemoryScanner::ScanMemory(std::function<bool(uintptr_t, const std::vector<uint8_t>&)> callback,
                                size_t chunkSize) const {
    if (!hProcess || !callback || chunkSize == 0) {
        return;
    }

    SYSTEM_INFO systemInfo{};
    GetNativeSystemInfo(&systemInfo);

    uintptr_t addr = reinterpret_cast<uintptr_t>(systemInfo.lpMinimumApplicationAddress);
    const uintptr_t maxAddress = reinterpret_cast<uintptr_t>(systemInfo.lpMaximumApplicationAddress);

    while (addr < maxAddress) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)) == sizeof(mbi)) {
            const uintptr_t regionStart = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            const size_t regionSize = mbi.RegionSize;

            if (IsReadableRegion(mbi)) {
                for (size_t offset = 0; offset < regionSize; offset += chunkSize) {
                    const size_t readSize = std::min(chunkSize, regionSize - offset);
                    auto data = ReadBytes(regionStart + offset, readSize);
                    if (!data.empty() && callback(regionStart + offset, data)) return;
                }
            }

            const uintptr_t next = regionStart + regionSize;
            if (next <= addr) {
                break;
            }
            addr = next;
        } else {
            if (maxAddress - addr < kFallbackAddressStep) {
                break;
            }
            addr += kFallbackAddressStep;
        }
    }
}
