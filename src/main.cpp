#include "memory_scanner.h"
#include "dynamic_finder.h"
#include <conio.h>

void PrintBanner() {
    Utils::SetColor(Utils::Color::CYAN);
    std::cout << R"(
╔══════════════════════════════════════════════════════════╗
║         Roblox Offset Scanner - Dynamic v2.0             ║
║              No Updates Needed - Fully Auto              ║
╚══════════════════════════════════════════════════════════╝
)" << std::endl;
    Utils::ResetColor();
}

int main() {
    SetConsoleTitleA("Roblox Offset Scanner - Dynamic");
    
    PrintBanner();
    
    // Check admin
    if (!Utils::IsProcessElevated()) {
        Utils::LogWarning("Not running as Administrator! Some features may not work.\n");
    }
    
    // Initialize scanner
    MemoryScanner scanner;
    
    while (!scanner.IsReady()) {
        Utils::LogInfo("Waiting for RobloxPlayerBeta.exe...");
        Utils::LogInfo("(Make sure Roblox is running, press Q to quit)\n");
        
        // Wait and retry
        for (int i = 0; i < 10; i++) {
            if (_kbhit() && (_getch() == 'q' || _getch() == 'Q')) {
                std::cout << "\nExiting...\n";
                return 0;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << ".";
        }
        std::cout << "\n";
        
        if (scanner.Initialize()) break;
    }
    
    // Create dynamic finder
    DynamicFinder finder(scanner);
    
    // Scan
    std::cout << "\n";
    auto results = finder.ScanAll();
    
    // Print results
    finder.PrintResults();
    
    // Export
    finder.ExportToFile("offsets.txt");
    std::cout << "\nResults saved to offsets.txt\n";
    
    // Wait
    std::cout << "\nPress any key to exit...";
    _getch();
    
    return 0;
}