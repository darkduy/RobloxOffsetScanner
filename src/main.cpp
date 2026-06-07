#include "memory_scanner.h"
#include "dynamic_finder.h"
#include <conio.h>
#include <cctype>
#include <iostream>
#include <string>

namespace {
    struct AppOptions {
        bool autoExit = false;
        bool showHelp = false;
        std::string exportFile = "offsets.txt";
        std::string headerFile;
    };

    void PrintBanner() {
        Utils::ScopedColor color(Utils::Color::CYAN);
        std::cout << R"(
╔══════════════════════════════════════════════════════════╗
║         Roblox Offset Scanner - Dynamic v2.0             ║
║              No Updates Needed - Fully Auto              ║
╚══════════════════════════════════════════════════════════╝
)" << std::endl;
    }

    void PrintHelp() {
        std::cout << "Usage: RobloxOffsetScanner.exe [options]\n\n"
                  << "Options:\n"
                  << "  --auto                 Scan and exit without waiting for a key press.\n"
                  << "  --export <file>        Write text/#define output (default: offsets.txt).\n"
                  << "  --export-header <file> Write a C++17 constexpr header.\n"
                  << "  --help                 Show this help text.\n";
    }

    bool ParseArgs(int argc, char* argv[], AppOptions& options) {
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--auto") {
                options.autoExit = true;
            } else if (arg == "--help" || arg == "-h") {
                options.showHelp = true;
            } else if (arg == "--export") {
                if (i + 1 >= argc) {
                    Utils::LogError("--export requires a file path.");
                    return false;
                }
                options.exportFile = argv[++i];
            } else if (arg == "--export-header") {
                if (i + 1 >= argc) {
                    Utils::LogError("--export-header requires a file path.");
                    return false;
                }
                options.headerFile = argv[++i];
            } else if (arg == "--verbose") {
                Utils::LogWarning("--verbose is accepted for compatibility; current logging is already verbose.");
            } else {
                Utils::LogError("Unknown argument: " + arg);
                return false;
            }
        }
        return true;
    }

    bool ShouldQuit() {
        if (!_kbhit()) {
            return false;
        }

        const int key = _getch();
        return std::tolower(key) == 'q';
    }
}

int main(int argc, char* argv[]) {
    SetConsoleTitleA("Roblox Offset Scanner - Dynamic");

    AppOptions options;
    if (!ParseArgs(argc, argv, options)) {
        PrintHelp();
        return 2;
    }

    if (options.showHelp) {
        PrintHelp();
        return 0;
    }

    PrintBanner();

    // Check admin
    if (!Utils::IsProcessElevated()) {
        Utils::LogWarning("Not running as Administrator! Some features may not work.\n");
    }

    // Initialize scanner
    MemoryScanner scanner;

    if (options.autoExit) {
        if (!scanner.Initialize()) {
            return 1;
        }
    } else {
        while (!scanner.IsReady()) {
            Utils::LogInfo("Waiting for RobloxPlayerBeta.exe...");
            Utils::LogInfo("(Make sure Roblox is running, press Q to quit)\n");

            // Wait and retry
            for (int i = 0; i < 10; i++) {
                if (ShouldQuit()) {
                    std::cout << "\nExiting...\n";
                    return 0;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << ".";
            }
            std::cout << "\n";

            if (scanner.Initialize()) break;
        }
    }

    // Create dynamic finder
    DynamicFinder finder(scanner);

    // Scan
    std::cout << "\n";
    finder.ScanAll();

    // Print results
    finder.PrintResults();

    // Export
    if (!options.exportFile.empty()) {
        finder.ExportToFile(options.exportFile);
    }
    if (!options.headerFile.empty()) {
        finder.ExportHeader(options.headerFile);
    }

    if (!options.autoExit) {
        std::cout << "\nPress any key to exit...";
        _getch();
    }

    return 0;
}
