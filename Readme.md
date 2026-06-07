# Roblox Offset Scanner - Dynamic

<div align="center">

![Version](https://img.shields.io/badge/version-2.1.0-blue)
![Build](https://img.shields.io/badge/build-passing-brightgreen)
![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Windows-blue)

**🔍 Fully Dynamic Roblox Offset Scanner - Không cần cập nhật thủ công**

[Features](#-features) • [Installation](#-installation) • [Usage](#-usage) • [How It Works](#-how-it-works) • [FAQ](#-faq)

</div>

---

## ⚠️ DISCLAIMER

**FOR EDUCATIONAL AND RESEARCH PURPOSES ONLY**

Công cụ này được tạo ra chỉ nhằm mục đích:
- 🔬 Nghiên cứu về reverse engineering
- 📚 Học tập về cấu trúc bộ nhớ
- 🛠️ Tìm hiểu cách hoạt động của game engine

**Sử dụng công cụ này để:**
- ❌ Exploit game
- ❌ Cheat/hack
- ❌ Mod game trái phép
- ❌ Vi phạm Terms of Service của Roblox

**CÓ THỂ DẪN ĐẾN BAN ACCOUNT VĨNH VIỄN!**

Tác giả không chịu trách nhiệm về bất kỳ hậu quả nào từ việc sử dụng tool này.

---

## 📖 Giới thiệu

Roblox Offset Scanner là công cụ **hoàn toàn tự động** (fully dynamic) để tìm các offset trong Roblox. Không giống như các tool khác sử dụng pattern byte cứng (hardcoded) và cần cập nhật thủ công sau mỗi bản update của Roblox, tool này:

- ✅ **Tự động phân tích** cấu trúc bộ nhớ
- ✅ **Không cần pattern byte** cố định
- ✅ **Tự thích nghi** với mọi phiên bản Roblox
- ✅ **Không cần cập nhật** thủ công
- ✅ **Scan real-time** khi Roblox đang chạy

## 🎯 Features

### Core Features
- 🔍 **Dynamic Memory Analysis**: Tự động tìm và phân tích cấu trúc bộ nhớ
- 🧠 **Heuristic Detection**: Sử dụng đặc điểm cấu trúc thay vì pattern byte
- 📊 **Confidence Scoring**: Đánh giá độ tin cậy của mỗi offset tìm được
- 🚀 **Fast Scanning**: Quét memory nhanh với chunk-based reading
- 📁 **Auto Export**: Tự động xuất kết quả ra file C++ header/text

### Offsets Scanned
| Offset | Mô tả | Phương pháp tìm |
|--------|-------|-----------------|
| `OFF_MAP_END` | Map end pointer | Phân tích tree structure |
| `OFF_MAP_LIST` | Map entry list | Pointer analysis |
| `OFF_MAP_MASK` | Hash mask | Bitmask detection |
| `OFF_ENTRY_FORWARD` | Entry linked list | Linked list traversal |
| `OFF_ENTRY_STRING` | Entry key string | String data detection |
| `OFF_ENTRY_GETSET` | Entry getter/setter | Function pointer detection |
| `OFF_STR_SIZE` | String length | Size field analysis |
| `OFF_STR_CAPACITY` | String capacity | Capacity field analysis |

## 📋 Requirements

### System Requirements
- **OS**: Windows 10/11 (64-bit)
- **RAM**: 4GB minimum
- **Storage**: 10MB free space
- **Permissions**: Administrator rights

### Dependencies
- RobloxPlayerBeta.exe (đang chạy)
- Visual C++ Redistributable (tự động include nếu build static)

## 📥 Installation

### Method 1: Download Pre-built (Recommended)

1. Vào [Releases](https://github.com/darkduy/RobloxOffsetScanner/releases)
2. Download `RobloxOffsetScanner.exe` từ bản release mới nhất
3. Không cần cài đặt gì thêm - portable executable

### Method 2: Build từ Source

```bash
# Clone repository
git clone https://github.com/darkduy/RobloxOffsetScanner.git
cd RobloxOffsetScanner

# Build với CMake
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --parallel

# Hoặc dùng build script
cd ..
build.bat
```

### Method 3: GitHub Actions Build

1. Fork repository này hoặc dùng trực tiếp repo [darkduy/RobloxOffsetScanner](https://github.com/darkduy/RobloxOffsetScanner)
2. Vào tab **Actions**
3. Chọn workflow **Build EXE**
4. Click **Run workflow**
5. Download artifact sau khi build xong


### Repository Improvements

Bản 2.1.0 bổ sung các cải tiến về maintainability và độ ổn định:

- CMake chỉ rõ project là Windows-only và bật warning chặt hơn.
- Parser command-line hiện hỗ trợ thực tế `--auto`, `--export`, `--export-header`, `--verbose`, và `--help`.
- Memory scanning bỏ qua region không đọc được/guarded và dùng giới hạn address từ hệ thống thay vì hard-code.
- Export header C++17 sử dụng `inline constexpr uintptr_t`.
- Báo cáo chi tiết thay đổi nằm ở [`docs/REPORT.md`](docs/REPORT.md).

## 🚀 Usage

### Quick Start

1. **Mở Roblox** trước
2. **Run as Administrator** `RobloxOffsetScanner.exe`
3. Đợi tool tự scan (5-15 giây)
4. Xem kết quả trên màn hình
5. Kết quả tự động lưu vào `offsets.txt`

### Command Line Options

```bash
# Chạy bình thường
RobloxOffsetScanner.exe

# Tự động scan và thoát
RobloxOffsetScanner.exe --auto --export offsets.txt

# Chỉ xuất C++ header
RobloxOffsetScanner.exe --export-header offsets.hpp

# Scan với verbose output
RobloxOffsetScanner.exe --verbose

# Hiển thị help
RobloxOffsetScanner.exe --help
```

### Output Format

#### Console Output
```
╔══════════════════════════════════════════════════════════╗
║         Roblox Offset Scanner - Dynamic v2.0             ║
║              No Updates Needed - Fully Auto              ║
╚══════════════════════════════════════════════════════════╝

[*] Waiting for RobloxPlayerBeta.exe...
[+] Connected to Roblox (PID: 12345)
[*] Base: 0x7FF600000000 Size: 150 MB

[*] Scanning MAP offsets...
[+] Found 47 map structures
[+] OFF_MAP_END = 0x10 (85%)
[+] OFF_MAP_LIST = 0x08 (92%)
[+] OFF_MAP_MASK = 0x7F (88%)

[*] Scanning ENTRY offsets...
[+] Found 156 entry structures
[+] OFF_ENTRY_FORWARD = 0x08 (90%)
[+] OFF_ENTRY_STRING = 0x10 (87%)
[+] OFF_ENTRY_GETSET = 0x18 (82%)

[*] Scanning STRING offsets...
[+] Found 234 string structures
[+] OFF_STR_SIZE = 0x10 (95%)
[+] OFF_STR_CAPACITY = 0x14 (91%)

======================================================================
                    SCAN RESULTS
======================================================================

           OFF_MAP_END : 0x10 (85%)
          OFF_MAP_LIST : 0x08 (92%)
          OFF_MAP_MASK : 0x7F (88%)
     OFF_ENTRY_FORWARD : 0x08 (90%)
      OFF_ENTRY_STRING : 0x10 (87%)
      OFF_ENTRY_GETSET : 0x18 (82%)
          OFF_STR_SIZE : 0x10 (95%)
      OFF_STR_CAPACITY : 0x14 (91%)

----------------------------------------------------------------------
Found: 8/8 offsets
======================================================================
```

#### File Export (offsets.txt)
```cpp
// Roblox Offsets - Auto-generated
// Generated by Dynamic Scanner v2.0

#define OFF_MAP_END 0x10
#define OFF_MAP_LIST 0x08
#define OFF_MAP_MASK 0x7F
#define OFF_ENTRY_FORWARD 0x08
#define OFF_ENTRY_STRING 0x10
#define OFF_ENTRY_GETSET 0x18
#define OFF_STR_SIZE 0x10
#define OFF_STR_CAPACITY 0x14
```

#### C++ Header (offsets.hpp)
```cpp
#pragma once

// Auto-generated Roblox Offsets
namespace RobloxOffsets {
    constexpr uintptr_t OFF_MAP_END = 0x10;
    constexpr uintptr_t OFF_MAP_LIST = 0x08;
    constexpr uintptr_t OFF_MAP_MASK = 0x7F;
    constexpr uintptr_t OFF_ENTRY_FORWARD = 0x08;
    constexpr uintptr_t OFF_ENTRY_STRING = 0x10;
    constexpr uintptr_t OFF_ENTRY_GETSET = 0x18;
    constexpr uintptr_t OFF_STR_SIZE = 0x10;
    constexpr uintptr_t OFF_STR_CAPACITY = 0x14;
}
```

## 🔬 How It Works

### Dynamic Scanning Methodology

Tool sử dụng **3 phương pháp chính** để tìm offset mà không cần pattern byte:

#### 1. Memory Structure Analysis
```
Tìm trong heap memory → Phân tích cấu trúc → Xác định type → Extract offset
```

- Quét tất cả vùng nhớ heap
- Tìm các block có cấu trúc giống map/string/entry
- Phân tích fields dựa trên giá trị (pointer, integer, string)

#### 2. Heuristic Detection
```
Nhận dạng đặc điểm → So sánh instances → Vote offset → Tính confidence
```

- **Map**: 2-4 pointers + 1-2 integers
- **String**: 1 pointer + 2 integers (size, capacity)
- **Entry**: Linked list node + function pointers

#### 3. Multi-Instance Comparison
```
Tìm nhiều instances → So sánh differences → Xác định fields → Validate
```

- Tìm 30-50 instances của mỗi structure
- So sánh để tìm pattern chung
- Vote offset xuất hiện nhiều nhất

### Algorithm Flow

```
Start
  ↓
Find Roblox Process
  ↓
Scan Heap Memory (chunks of 1MB)
  ↓
┌─────────────────────────────────┐
│  Find Map Structures            │
│  - Look for 2-4 ptrs + ints     │
│  - Analyze tree nodes           │
│  - Extract: END, LIST, MASK     │
└─────────────────────────────────┘
  ↓
┌─────────────────────────────────┐
│  Find Entry Structures          │
│  - Traverse map trees           │
│  - Find linked list nodes       │
│  - Extract: FORWARD, STRING,    │
│    GETSET                       │
└─────────────────────────────────┘
  ↓
┌─────────────────────────────────┐
│  Find String Structures         │
│  - Look for [ptr][size][cap]    │
│  - Validate ASCII data          │
│  - Extract: SIZE, CAPACITY      │
└─────────────────────────────────┘
  ↓
Vote & Calculate Confidence
  ↓
Export Results
  ↓
End
```

## 📊 Confidence Scoring

Mỗi offset được gán confidence score (0-100%) dựa trên:

| Factor | Weight | Description |
|--------|--------|-------------|
| Frequency | 40% | Số lần offset xuất hiện trong các instances |
| Consistency | 30% | Độ nhất quán giữa các instances |
| Validation | 20% | Pass các validation checks |
| Heuristic | 10% | Match với đặc điểm mong đợi |

- 🟢 **>80%**: High confidence - Gần như chắc chắn đúng
- 🟡 **60-80%**: Medium confidence - Có thể đúng
- 🔴 **<60%**: Low confidence - Cần verify thêm

## 🔧 Troubleshooting

### Common Issues

| Issue | Solution |
|-------|----------|
| **"Roblox not found"** | Mở Roblox trước khi chạy tool |
| **"Access denied"** | Chạy với Administrator rights |
| **"Not found" offsets** | Thử scan lại hoặc restart Roblox |
| **Chậm** | Đóng các ứng dụng không cần thiết |
| **Sai offset** | Kiểm tra confidence score, dùng offset có score cao |

### Debug Mode

Thêm `--verbose` để xem chi tiết quá trình scan:
```bash
RobloxOffsetScanner.exe --verbose
```

## 🏗️ Project Structure

```
RobloxOffsetScanner/
├── .github/
│   └── workflows/
│       └── build.yml              # GitHub Actions build
├── src/
│   ├── main.cpp                   # Entry point
│   ├── memory_scanner.h/cpp       # Memory reading engine
│   ├── dynamic_finder.h/cpp       # Dynamic offset finder
│   └── utils.h                    # Utility functions
├── CMakeLists.txt                 # Build configuration
├── build.bat                      # Build script
├── README.md                      # Documentation
└── .gitignore
```

## 🤝 Contributing

### Cách đóng góp

1. Fork repository tại [darkduy/RobloxOffsetScanner](https://github.com/darkduy/RobloxOffsetScanner)
2. Tạo branch mới (`git checkout -b feature/improvement`)
3. Commit thay đổi (`git commit -m 'Add new feature'`)
4. Push lên branch (`git push origin feature/improvement`)
5. Tạo Pull Request

### Cần giúp đỡ về
- 📝 Cải thiện heuristic detection
- 🚀 Tối ưu tốc độ scan
- 🐛 Bug fixes
- 📚 Documentation

## ❓ FAQ

<details>
<summary><b>Q: Tool có cần cập nhật sau mỗi Roblox update không?</b></summary>
<b>A:</b> KHÔNG! Tool được thiết kế để tự động thích nghi với mọi phiên bản Roblox bằng cách phân tích cấu trúc memory thay vì dùng pattern byte cố định.
</details>

<details>
<summary><b>Q: Tại sao một số offset không tìm thấy?</b></summary>
<b>A:</b> Có thể do:
- Roblox chưa load đủ data (chơi game vài phút rồi thử lại)
- Cấu trúc thay đổi quá nhiều (tool sẽ cần thêm thời gian để thích nghi)
- Anti-cheat can thiệp (thử tắt antivirus tạm thời)
</details>

<details>
<summary><b>Q: Confidence score bao nhiêu là đủ?</b></summary>
<b>A:</b> Trên 80% là có thể dùng được. Dưới 60% nên scan lại hoặc verify bằng cách khác.
</details>

<details>
<summary><b>Q: Tool có bị detect bởi Byfron không?</b></summary>
<b>A:</b> Tool chỉ READ memory, không WRITE hay INJECT gì cả. Tuy nhiên vẫn có rủi ro bị detect. Sử dụng cẩn thận.
</details>

<details>
<summary><b>Q: Có scan được Roblox 32-bit không?</b></summary>
<b>A:</b> Hiện tại chỉ hỗ trợ 64-bit. Roblox đã ngừng hỗ trợ 32-bit từ 2023.
</details>

## 📜 License

MIT License - Xem file [LICENSE](LICENSE) để biết thêm chi tiết.

## ⭐ Star History

[![Star History Chart](https://api.star-history.com/svg?repos=darkduy/RobloxOffsetScanner&type=Date)](https://star-history.com/#darkduy/RobloxOffsetScanner&Date)

## 📞 Contact

- **GitHub**: [darkduy/RobloxOffsetScanner](https://github.com/darkduy/RobloxOffsetScanner)
- **Issues**: [GitHub Issues](https://github.com/darkduy/RobloxOffsetScanner/issues)
- **Discussions**: [GitHub Discussions](https://github.com/darkduy/RobloxOffsetScanner/discussions)

---

<div align="center">

**Made with ❤️ by [darkduy](https://github.com/darkduy)**

*For Educational Purposes Only*

</div>