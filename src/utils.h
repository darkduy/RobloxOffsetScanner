#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace Utils {
    enum class Color : WORD {
        RED = 12,
        GREEN = 10,
        YELLOW = 14,
        BLUE = 9,
        CYAN = 11,
        WHITE = 15,
        GRAY = 8
    };

    inline void SetColor(Color color) {
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        if (console != INVALID_HANDLE_VALUE) {
            SetConsoleTextAttribute(console, static_cast<WORD>(color));
        }
    }

    inline void ResetColor() { SetColor(Color::WHITE); }

    class ScopedColor {
    public:
        explicit ScopedColor(Color color) { SetColor(color); }
        ~ScopedColor() { ResetColor(); }

        ScopedColor(const ScopedColor&) = delete;
        ScopedColor& operator=(const ScopedColor&) = delete;
    };

    inline void LogInfo(const std::string& msg) {
        { ScopedColor color(Color::CYAN); std::cout << "[*] "; }
        std::cout << msg << std::endl;
    }

    inline void LogSuccess(const std::string& msg) {
        { ScopedColor color(Color::GREEN); std::cout << "[+] "; }
        std::cout << msg << std::endl;
    }

    inline void LogError(const std::string& msg) {
        { ScopedColor color(Color::RED); std::cout << "[-] "; }
        std::cout << msg << std::endl;
    }

    inline void LogWarning(const std::string& msg) {
        { ScopedColor color(Color::YELLOW); std::cout << "[!] "; }
        std::cout << msg << std::endl;
    }

    inline std::string ToHex(uintptr_t value) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << value;
        return ss.str();
    }

    template<typename T>
    inline std::optional<T> ReadUnaligned(const std::vector<uint8_t>& data, size_t offset) {
        if (offset + sizeof(T) > data.size()) {
            return std::nullopt;
        }

        T value{};
        std::memcpy(&value, data.data() + offset, sizeof(T));
        return value;
    }

    inline bool IsProcessElevated() {
        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
            return false;
        }

        TOKEN_ELEVATION elevation{};
        DWORD size = sizeof(TOKEN_ELEVATION);
        const bool ok = GetTokenInformation(token, TokenElevation, &elevation, size, &size) != FALSE;
        CloseHandle(token);
        return ok && elevation.TokenIsElevated;
    }
}
