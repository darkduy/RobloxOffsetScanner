#include "dynamic_scanner.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <conio.h>

class ScannerApp {
private:
    MemoryScanner scanner;
    DynamicOffsetScanner* dynScanner;
    bool autoRescan;
    int scanInterval; // seconds

public:
    ScannerApp() : dynScanner(nullptr), autoRescan(false), scanInterval(30) {}

    void Run() {
        PrintBanner();
        
        if (!CheckAdminRights()) {
            Utils::LogWarning("Not running as administrator. Some features may not work.");
            std::cout << std::endl;
        }

        while (true) {
            if (!InitializeScanner()) {
                if (!WaitForRoblox()) {
                    break;
                }
                continue;
            }

            ShowMenu();
            char choice;
            std::cin >> choice;
            std::cin.ignore();

            switch (choice) {
                case '1':
                    PerformScan();
                    break;
                case '2':
                    autoRescan = !autoRescan;
                    if (autoRescan) {
                        SetAutoRescanInterval();
                        StartAutoRescan();
                    }
                    break;
                case '3':
                    ExportResults();
                    break;
                case '4':
                    ShowDetailedInfo();
                    break;
                case '5':
                    std::cout << "Exiting...\n";
                    return;
                default:
                    Utils::LogWarning("Invalid option");
            }
        }
    }

private:
    void PrintBanner() {
        Utils::SetColor(Utils::Color::CYAN);
        std::cout << R"(
╔══════════════════════════════════════════════════════════╗
║         Roblox Offset Scanner v2.0 - Dynamic             ║
║                 By Security Researcher                    ║
╚══════════════════════════════════════════════════════════╝
        )" << std::endl;
        Utils::ResetColor();
    }

    bool CheckAdminRights() {
        BOOL isElevated = FALSE;
        HANDLE hToken = NULL;
        
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elevation;
            DWORD size = sizeof(TOKEN_ELEVATION);
            if (GetTokenInformation(hToken, TokenElevation, &elevation, size, &size)) {
                isElevated = elevation.TokenIsElevated;
            }
            CloseHandle(hToken);
        }
        
        return isElevated;
    }

    bool InitializeScanner() {
        if (!scanner.IsInitialized()) {
            Utils::LogInfo("Looking for RobloxPlayerBeta.exe...");
            if (!scanner.Initialize()) {
                return false;
            }
            dynScanner = new DynamicOffsetScanner(scanner);
        }
        return true;
    }

    bool WaitForRoblox() {
        Utils::LogInfo("Waiting for Roblox to start... (Press 'Q' to quit)");
        Utils::LogInfo("Auto-refreshing every 5 seconds...");
        
        while (!scanner.IsInitialized()) {
            if (_kbhit()) {
                char c = _getch();
                if (c == 'q' || c == 'Q') {
                    return false;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            if (scanner.Initialize()) {
                dynScanner = new DynamicOffsetScanner(scanner);
                return true;
            }
        }
        return true;
    }

    void ShowMenu() {
        std::cout << "\n";
        Utils::SetColor(Utils::Color::YELLOW);
        std::cout << "=== MENU ===" << std::endl;
        Utils::ResetColor();
        std::cout << "1. Scan Offsets (Once)\n";
        std::cout << "2. Toggle Auto-Rescan (" << (autoRescan ? "ON" : "OFF") << ")\n";
        std::cout << "3. Export Results\n";
        std::cout << "4. Show Detailed Info\n";
        std::cout << "5. Exit\n";
        std::cout << "Choice: ";
    }

    void PerformScan() {
        if (!dynScanner) return;
        
        std::cout << "\n";
        Utils::LogInfo("Performing dynamic scan...\n");
        
        // Show progress animation
        std::thread progressThread([this]() {
            const char spinner[] = {'|', '/', '-', '\\'};
            int i = 0;
            while (dynScanner->IsRunning()) {
                float progress = dynScanner->GetProgress();
                std::cout << "\r" << spinner[i % 4] << " Progress: " 
                         << (int)(progress * 100) << "%";
                i++;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            std::cout << "\r                        \r";
        });

        dynScanner->ScanAll();
        
        progressThread.join();
        
        dynScanner->PrintResults();
    }

    void SetAutoRescanInterval() {
        std::cout << "Enter rescan interval (seconds): ";
        std::cin >> scanInterval;
        std::cin.ignore();
        
        if (scanInterval < 5) scanInterval = 5;
        if (scanInterval > 3600) scanInterval = 3600;
        
        Utils::LogInfo("Auto-rescan interval set to " + std::to_string(scanInterval) + " seconds");
    }

    void StartAutoRescan() {
        std::thread([this]() {
            while (autoRescan && scanner.IsInitialized()) {
                std::this_thread::sleep_for(std::chrono::seconds(scanInterval));
                
                if (autoRescan) {
                    Utils::LogInfo("Auto-rescanning...");
                    dynScanner->ScanAll();
                    dynScanner->PrintResults();
                    
                    // Auto-export
                    dynScanner->ExportToFile("offsets_auto.txt");
                    dynScanner->ExportToHeader("offsets_auto.hpp");
                }
            }
        }).detach();
        
        Utils::LogSuccess("Auto-rescan enabled (every " + std::to_string(scanInterval) + "s)");
    }

    void ExportResults() {
        if (!dynScanner) return;
        
        std::cout << "\nExport Options:\n";
        std::cout << "1. Text file (offsets.txt)\n";
        std::cout << "2. C++ Header (offsets.hpp)\n";
        std::cout << "3. Both\n";
        std::cout << "Choice: ";
        
        char choice;
        std::cin >> choice;
        std::cin.ignore();
        
        switch (choice) {
            case '1':
                dynScanner->ExportToFile("offsets.txt");
                break;
            case '2':
                dynScanner->ExportToHeader("offsets.hpp");
                break;
            case '3':
                dynScanner->ExportToFile("offsets.txt");
                dynScanner->ExportToHeader("offsets.hpp");
                break;
            default:
                Utils::LogError("Invalid choice");
        }
    }

    void ShowDetailedInfo() {
        if (!dynScanner) {
            Utils::LogWarning("No scan performed yet");
            return;
        }

        auto results = dynScanner->GetAllResults();
        
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "DETAILED OFFSET INFORMATION" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        for (const auto& result : results) {
            std::cout << "\nOffset: " << result.name << std::endl;
            std::cout << std::string(40, '-') << std::endl;
            std::cout << "  Status:     " << (result.found ? "FOUND" : "NOT FOUND") << std::endl;
            if (result.found) {
                std::cout << "  Value:      " << Utils::ToHex(result.offset) << std::endl;
                std::cout << "  Decimal:    " << std::dec << result.offset << std::endl;
                std::cout << "  Confidence: " << (int)(result.confidence * 100) << "%" << std::endl;
                std::cout << "  Method:     " << result.method << std::endl;
            }
        }
        std::cout << std::string(80, '=') << std::endl;
    }
};

int main() {
    // Set console title
    SetConsoleTitleA("Roblox Offset Scanner v2.0 - Dynamic");
    
    // Set console encoding to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    
    try {
        ScannerApp app;
        app.Run();
    } catch (const std::exception& e) {
        Utils::LogError(std::string("Fatal error: ") + e.what());
        std::cout << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }
    
    return 0;
}