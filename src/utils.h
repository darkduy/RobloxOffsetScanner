#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>

namespace Utils {
    enum class Color {
        RED = 12, GREEN = 10, YELLOW = 14, BLUE = 9, CYAN = 11, WHITE = 15, GRAY = 8
    };

    inline void SetColor(Color color) {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), static_cast<WORD>(color));
    }
    inline void ResetColor() { SetColor(Color::WHITE); }
    
    inline void LogInfo(const std::string& msg) {
        SetColor(Color::CYAN); std::cout << "[*] "; ResetColor();
        std::cout << msg << std::endl;
    }
    inline void LogSuccess(const std::string& msg) {
        SetColor(Color::GREEN); std::cout << "[+] "; ResetColor();
        std::cout << msg << std::endl;
    }
    inline void LogError(const std::string& msg) {
        SetColor(Color::RED); std::cout << "[-] "; ResetColor();
        std::cout << msg << std::endl;
    }
    inline void LogWarning(const std::string& msg) {
        SetColor(Color::YELLOW); std::cout << "[!] "; ResetColor();
        std::cout << msg << std::endl;
    }

    inline std::string ToHex(uintptr_t value) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << value;
        return ss.str();
    }

    inline bool IsProcessElevated() {
        HANDLE hToken = NULL;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elevation;
            DWORD size = sizeof(TOKEN_ELEVATION);
            if (GetTokenInformation(hToken, TokenElevation, &elevation, size, &size)) {
                CloseHandle(hToken);
                return elevation.TokenIsElevated;
            }
            CloseHandle(hToken);
        }
        return false;
    }
}