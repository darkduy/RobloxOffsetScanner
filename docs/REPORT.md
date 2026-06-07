# Báo cáo cải tiến repository RobloxOffsetScanner

Ngày thực hiện: 2026-06-06

## 1. Tóm tắt

Repository đã được rà soát và cải tiến theo hướng an toàn hơn khi build/chạy trên Windows, dễ bảo trì hơn, và khớp tốt hơn với phần hướng dẫn sử dụng trong README. Các thay đổi tập trung vào:

- Chuẩn hóa cấu hình CMake và cảnh báo biên dịch.
- Cải thiện quản lý handle/process và đọc bộ nhớ an toàn hơn.
- Bổ sung parser command-line thật sự cho các tùy chọn đã mô tả.
- Giảm rủi ro vòng quét/đệ quy không giới hạn khi gặp vùng nhớ hoặc cấu trúc ứng viên không hợp lệ.
- Bổ sung export dạng C++ header `inline constexpr` bên cạnh file text `#define`.
- Cập nhật tài liệu để phản ánh trạng thái hiện tại của project.

## 2. Thay đổi theo khu vực

### 2.1 CMake/build system

- Nâng metadata project lên `VERSION 2.1.0`.
- Thêm chặn build rõ ràng cho môi trường không phải Windows vì code phụ thuộc Win32 API.
- Đưa toàn bộ header vào target để IDE hiển thị đầy đủ cấu trúc source.
- Bật C++17 không dùng compiler extension.
- Bật warning mặc định:
  - MSVC: `/W4 /permissive- /EHsc`.
  - Compiler khác: `-Wall -Wextra -Wpedantic`.
- Chuẩn hóa output executable về thư mục `build/bin`.

### 2.2 Tiện ích console và helper

- Thêm `Utils::ScopedColor` để tự động reset màu console khi thoát scope.
- Cải thiện hàm log để hạn chế lỗi quên reset màu.
- Thêm `Utils::ReadUnaligned<T>` để đọc số nguyên từ buffer byte bằng `std::memcpy`, tránh type-punning/alignment undefined behavior.
- Cải thiện `IsProcessElevated()` để đóng token theo một đường đi rõ ràng hơn.

### 2.3 MemoryScanner

- Thêm destructor gọi `Close()` và chặn copy object để tránh double-close handle.
- `Initialize()` giờ đóng trạng thái cũ trước khi mở lại process và tự dọn handle nếu lấy module thất bại.
- `Read<T>()` chỉ trả về dữ liệu khi `ReadProcessMemory` đọc đủ số byte cần thiết.
- `ReadBytes()` xử lý input rỗng/chưa init và chỉ trả dữ liệu khi đọc được ít nhất một byte.
- `ReadString()` tìm null terminator trong byte buffer thay vì phụ thuộc trực tiếp vào constructor C-string.
- `ScanMemory()` dùng `GetNativeSystemInfo()` để lấy dải địa chỉ hợp lệ thay vì hard-code giới hạn địa chỉ.
- Bỏ qua vùng `PAGE_GUARD`, `PAGE_NOACCESS`, và chỉ quét các protection có thể đọc.
- Thêm guard chống overflow khi chuyển sang region tiếp theo.

### 2.4 DynamicFinder

- Cache kết quả tìm map structures để tránh scan bộ nhớ lại nhiều lần trong cùng một lượt chạy.
- Thêm giới hạn mẫu cho map/string/entry structures để giảm nguy cơ scan quá lâu trên dữ liệu nhiễu.
- Thay các lần đọc `*(uint64_t*)` trên buffer bằng helper đọc unaligned an toàn hơn.
- Giới hạn traversal entry tree và kiểm tra lại node có nằm trong heap memory trước khi duyệt.
- Sửa logic nhận diện map sentinel/end node vốn có nhánh khó đạt được do điều kiện trước đó mâu thuẫn.
- Thêm export C++ header với `inline constexpr uintptr_t`.
- `ExportToFile()` và `ExportHeader()` trả về `bool` để caller có thể biết thao tác export có thành công hay không.

### 2.5 Command-line interface

Đã triển khai parser cho các tùy chọn:

```text
--auto
--export <file>
--export-header <file>
--verbose
--help
```

Các cải tiến chính:

- `--help` in hướng dẫn và thoát với mã `0`.
- `--auto` chạy non-interactive: nếu không kết nối được Roblox thì thoát với mã lỗi thay vì chờ phím vô hạn.
- `--export <file>` cho phép đổi đường dẫn file text output.
- `--export-header <file>` tạo header C++17.
- Sửa lỗi đọc phím `Q`: trước đây gọi `_getch()` hai lần trong cùng điều kiện, có thể nuốt phím sai.

## 3. Kiểm tra đã thực hiện

- `cmake -S . -B build`: kiểm tra cấu hình CMake. Kết quả cảnh báo/giới hạn môi trường vì container hiện tại là Linux, trong khi project dùng Win32 API và CMake đã chủ động chặn build ngoài Windows.
- `git diff --check`: kiểm tra whitespace và patch format. Kết quả đạt.

## 4. Giới hạn còn lại

- Chưa thể build/run executable trong container hiện tại vì thiếu môi trường Windows/Win32 API.
- Cần chạy thêm trên Windows với Visual Studio 2022 hoặc toolchain MSVC để xác thực compile/runtime đầy đủ.
- Kết quả quét offset phụ thuộc tiến trình Roblox đang chạy và quyền đọc memory của process tại thời điểm chạy.

## 5. Đề xuất tiếp theo

- Thêm CI Windows build bằng GitHub Actions.
- Tách `DynamicFinder` thành các module nhỏ hơn nếu số lượng offset cần scan tăng lên.
- Bổ sung unit test cho các helper thuần C++ không phụ thuộc Win32 API.
- Thêm structured output JSON để dễ tích hợp với tooling khác.
