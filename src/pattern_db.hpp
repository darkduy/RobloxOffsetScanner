#pragma once
#include <vector>
#include <string>
#include <map>

// Pattern database for different Roblox versions
struct PatternDB {
    struct OffsetPattern {
        std::string name;
        std::vector<std::string> patterns; // Multiple patterns for different versions
        std::string mask;
        int offsetAdjust; // Adjust offset if needed
    };

    static std::vector<OffsetPattern> GetPatterns() {
        return {
            // OFF_MAP_END patterns
            {"OFF_MAP_END", {
                "48 8B 47 ? 48 89 45",     // mov rax, [rdi+XX]
                "48 8B 87 ? ? ? ? 48 89",   // mov rax, [rdi+XXXXXXXX]
                "8B 47 ? 48 89 45"          // mov eax, [rdi+XX]
            }, "", 0},

            // OFF_MAP_LIST patterns
            {"OFF_MAP_LIST", {
                "48 8D 4F ? 48 8B D7",      // lea rcx, [rdi+XX]
                "48 8B 47 ? 48 8B CF",      // mov rax, [rdi+XX]; mov rcx, rdi
                "48 8D 47 ? 48 89 45"       // lea rax, [rdi+XX]
            }, "", 0},

            // OFF_MAP_MASK patterns
            {"OFF_MAP_MASK", {
                "83 E0 ? 48 8B 4C",          // and eax, XX
                "25 ? ? ? ? 48 8B",          // and eax, XXXXXXXX
                "83 E1 ? 48 8B"             // and ecx, XX
            }, "", 0},

            // OFF_ENTRY_FORWARD patterns
            {"OFF_ENTRY_FORWARD", {
                "48 8B 41 ? 48 85 C0",      // mov rax, [rcx+XX]
                "48 8B 47 ? 48 85 C0 74",   // mov rax, [rdi+XX]
                "48 8B 41 ? 48 89 45"       // mov rax, [rcx+XX]
            }, "", 0},

            // OFF_ENTRY_STRING patterns
            {"OFF_ENTRY_STRING", {
                "48 8B 41 ? 48 8B 50",       // mov rax, [rcx+XX]
                "48 8B 47 ? 48 8B 48",       // mov rax, [rdi+XX]
                "48 8B 47 ? 48 8B 4F"       // mov rax, [rdi+XX]; mov rcx, [rdi+XX]
            }, "", 0},

            // OFF_ENTRY_GETSET patterns
            {"OFF_ENTRY_GETSET", {
                "48 8B 41 ? 48 85 C0 0F",   // mov rax, [rcx+XX]
                "48 8B 47 ? 48 85 C0 0F",   // mov rax, [rdi+XX]
                "4C 8B 41 ? 4D 85 C0"       // mov r8, [rcx+XX]
            }, "", 0},

            // OFF_STR_SIZE patterns
            {"OFF_STR_SIZE", {
                "83 78 ? 00 7C",             // cmp dword ptr [rax+XX], 0
                "83 79 ? 00 7C",             // cmp dword ptr [rcx+XX], 0
                "8B 47 ? 85 C0"             // mov eax, [rdi+XX]
            }, "", 0},

            // OFF_STR_CAPACITY patterns
            {"OFF_STR_CAPACITY", {
                "3B 47 ? 7C",               // cmp eax, [rdi+XX]
                "3B 41 ? 7C",               // cmp eax, [rcx+XX]
                "8B 47 ? 3B 47"            // mov eax, [rdi+XX]; cmp eax, [rdi+XX]
            }, "", 0}
        };
    }

    // Common offset values per Roblox version
    struct VersionOffsets {
        std::string version;
        std::map<std::string, uintptr_t> offsets;
    };

    static std::vector<VersionOffsets> GetKnownOffsets() {
        return {
            {"v550", {
                {"OFF_MAP_END", 0x10},
                {"OFF_MAP_LIST", 0x08},
                {"OFF_MAP_MASK", 0x7F},
                {"OFF_ENTRY_FORWARD", 0x08},
                {"OFF_ENTRY_STRING", 0x10},
                {"OFF_ENTRY_GETSET", 0x18},
                {"OFF_STR_SIZE", 0x10},
                {"OFF_STR_CAPACITY", 0x14}
            }},
            {"v555", {
                {"OFF_MAP_END", 0x14},
                {"OFF_MAP_LIST", 0x0C},
                {"OFF_MAP_MASK", 0x7F},
                {"OFF_ENTRY_FORWARD", 0x0C},
                {"OFF_ENTRY_STRING", 0x14},
                {"OFF_ENTRY_GETSET", 0x1C},
                {"OFF_STR_SIZE", 0x14},
                {"OFF_STR_CAPACITY", 0x18}
            }}
        };
    }
};