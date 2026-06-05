#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <fstream>
#include <algorithm>

namespace Utils {
    // Console colors
    enum class Color {
        RED = 12,
        GREEN = 10,
        YELLOW = 14,
        BLUE = 9,
        CYAN = 11,
        WHITE = 15,
        GRAY = 8
    };

    inline void SetColor(Color color) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, static_cast<WORD>(color));
    }

    inline void ResetColor() {
        SetColor(Color::WHITE);
    }

    // Logging
    inline void LogInfo(const std::string& msg) {
        SetColor(Color::CYAN);
        std::cout << "[*] ";
        ResetColor();
        std::cout << msg << std::endl;
    }

    inline void LogSuccess(const std::string& msg) {
        SetColor(Color::GREEN);
        std::cout << "[+] ";
        ResetColor();
        std::cout << msg << std::endl;
    }

    inline void LogError(const std::string& msg) {
        SetColor(Color::RED);
        std::cout << "[-] ";
        ResetColor();
        std::cout << msg << std::endl;
    }

    inline void LogWarning(const std::string& msg) {
        SetColor(Color::YELLOW);
        std::cout << "[!] ";
        ResetColor();
        std::cout << msg << std::endl;
    }

    // Memory operations
    inline bool IsValidPointer(HANDLE hProcess, uintptr_t address) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(hProcess, (LPCVOID)address, &mbi, sizeof(mbi)) == 0)
            return false;
        
        return (mbi.State == MEM_COMMIT && 
                (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)));
    }

    // String operations
    inline std::vector<std::string> Split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    inline std::string ToHex(uintptr_t value) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << value;
        return ss.str();
    }

    // Pattern conversion
    inline std::vector<uint8_t> ParsePattern(const std::string& pattern, std::string& mask) {
        std::vector<uint8_t> bytes;
        mask.clear();
        
        auto tokens = Split(pattern, ' ');
        for (const auto& token : tokens) {
            if (token == "?" || token == "??") {
                bytes.push_back(0);
                mask += '?';
            } else {
                bytes.push_back(static_cast<uint8_t>(std::stoi(token, nullptr, 16)));
                mask += 'x';
            }
        }
        
        return bytes;
    }
}