# 09 — Dựng môi trường, build, nạp bo

File này để người chưa từng đụng project vẫn build và nạp được.

## 1. Cần cài gì

| Công cụ | Để làm gì | Bắt buộc |
|---|---|---|
| `arm-none-eabi-gcc` | Biên dịch cho ARM Cortex-M | Có |
| `make` | Chạy Makefile | Có |
| STM32CubeProgrammer | Nạp qua ST-Link | Chọn 1 trong 2 |
| `ak-flash` | Nạp qua UART, xài bootloader | Chọn 1 trong 2 |
| `minicom` | Xem log UART | Nên có |
| OpenOCD + `arm-none-eabi-gdb` | Debug từng bước | Không bắt buộc |

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install gcc-arm-none-eabi build-essential minicom
```

### macOS

```bash
brew install --cask gcc-arm-embedded
brew install make minicom
```

### Kiểm tra cài xong chưa

```bash
arm-none-eabi-gcc --version
make --version
```

Ra số phiên bản là được.

## 2. Makefile tìm toolchain ở đâu

Mở `application/Makefile`, dòng 16:

```makefile
GCC_PATH ?= /usr
```

Rồi dòng 75 ghép thành:

```makefile
CC = $(GCC_PATH)/bin/arm-none-eabi-gcc
```

Nên mặc định nó tìm ở `/usr/bin/arm-none-eabi-gcc`.

Cài chỗ khác thì truyền vào lúc build:

```bash
make GCC_PATH=/opt/gcc-arm-none-eabi
```

Dấu `?=` nghĩa là "chỉ gán nếu chưa có giá trị", nên biến môi trường hay tham số dòng lệnh đều đè lên được.

Còn ST-Link thì ở dòng 17:

```makefile
PROGRAMER_PATH = $(HOME)/Workspace/Tools/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin
```

Cái này **hardcode theo máy người viết gốc**. Máy em gần như chắc chắn khác. Sửa dòng đó, hoặc truyền vào:

```bash
make flash PROGRAMER_PATH=/đường/dẫn/tới/STM32CubeProgrammer/bin
```

## 3. Build

```bash
cd application
make
```

Build xong ra thư mục `build_dungeon-game/`:

| File | Là gì |
|---|---|
| `dungeon-game.axf` | ELF có ký hiệu debug, dùng cho gdb |
| `dungeon-game.bin` | Ảnh nhị phân thuần, cái này đem nạp |
| `dungeon-game.map` | Bản đồ bộ nhớ, xem cái gì chiếm bao nhiêu |
| `*.o` | Object từng file nguồn |
| `*.su` | Stack usage từng hàm |
| `*.d` | Phụ thuộc header, để make biết cần build lại gì |

Build nhanh hơn thì chạy song song:

```bash
make -j4
```

### Sau khi sửa file .h hay xoá file

```bash
make clean
make
```

Bắt buộc. Vì mấy file `.d` cũ vẫn trỏ tới đường dẫn cũ. Xoá một thư mục driver mà không `make clean` là lỗi "No rule to make target".

### Các lệnh khác

```bash
make help     # in danh sách lệnh
make clean    # xoá thư mục build
make asm      # sinh file .asm đã disassemble
make sym      # liệt kê ký hiệu từ object
```

## 4. Đọc con số dung lượng

Cuối bản build có dòng size. Muốn xem lại:

```bash
arm-none-eabi-size build_dungeon-game/dungeon-game.axf
```

```raw
   text    data     bss     dec     hex filename
  80540    2120    9844   92504   16958 dungeon-game.axf
```

Cách hiểu:

- `text` là code + hằng số, nằm ở **flash**
- `data` là biến toàn cục có giá trị khởi tạo. Chiếm **cả flash lẫn RAM**, vì giá trị ban đầu nằm trong flash rồi được copy sang RAM lúc khởi động
- `bss` là biến toàn cục bằng 0, chỉ chiếm **RAM**

Vậy:

```raw
Flash dùng = text + data = 80540 + 2120 = 82660 byte / 118784 (116 KB)  ≈ 70 %
RAM  dùng  = data + bss  = 2120 + 9844  = 11964 byte / 16384 (16 KB)    ≈ 73 %
```

Cộng thêm heap 2 KB thì RAM lên khoảng 85 %.

Quá 100 % thì linker báo lỗi `region FLASH overflowed` hoặc `region SRAM overflowed`.

## 5. Nạp bo, cách 1: ST-Link

Cần cáp ST-Link nối vào chân SWD.

```bash
cd application
make flash
```

Bên trong nó chạy:

```bash
$(PROGRAMER_PATH)/STM32_Programmer.sh -c port=SWD \
    -w build_dungeon-game/dungeon-game.bin 0x08003000 -rst
```

Để ý địa chỉ `0x08003000`. Đó là chỗ application bắt đầu, **không phải** `0x08000000`. Nạp nhầm vào `0x08000000` là đè mất bootloader.

Có `st-flash` (gói stlink-tools) thì cũng được:

```bash
st-flash --reset write build_dungeon-game/dungeon-game.bin 0x08003000
```

### Nạp bootloader

Bootloader build riêng, nạp riêng, vào địa chỉ khác:

```bash
cd boot
make
make flash        # nạp vào 0x08000000
```

Bình thường chỉ nạp một lần. Sau đó chỉ cần nạp application.

## 6. Nạp bo, cách 2: UART qua bootloader

Không cần ST-Link, chỉ cần cáp USB-UART.

Cài `ak-flash` trước, xem https://github.com/ak-embedded-software/ak-flash

```bash
cd application
make flash dev=/dev/ttyUSB0
```

Trên macOS thì cổng thường là `/dev/tty.usbserial-XXXX`.

Cách xem cổng nào:

```bash
ls /dev/ttyUSB*        # Linux
ls /dev/tty.usb*       # macOS
```

### Bo không chịu vào chế độ nạp

Giữ nút **MODE** trong lúc cấp nguồn hoặc lúc nhấn Reset.

Bootloader kiểm tra nút này ngay lúc khởi động, ở `boot/sources/app/uart_boot.cpp`:

```c
uint32_t uart_boot_is_required() {
    for (int i = 0; i < 300; i++) {
        if (io_button_mode_read()) return 0;   /* nhả ra là thôi */
    }
    return 1;                                  /* giữ suốt thì vào chế độ nạp */
}
```

Vào được rồi thì LED life nháy nhanh, chu kỳ 250 ms.

Đây cũng là đường cứu khi nạp nhầm firmware hỏng làm bo crash liên tục.

### Nạp lại firmware gốc

```bash
make factory dev=/dev/ttyUSB0
```

Nó nạp `resources/bin/ak-base-kit-stm32l151-application.bin`, tức bản demo gốc của bo.

## 7. Xem log UART

```bash
cd application
make com dev=/dev/ttyUSB0
```

Bên trong là `minicom -D /dev/ttyUSB0 -b 115200`.

Baud rate 115200. Khai ở `application/Makefile`, biến `SYS_CONSOLE_BAUDRATE`.

Thoát minicom: `Ctrl-A` rồi `X`.

Không thích minicom thì dùng gì cũng được:

```bash
screen /dev/ttyUSB0 115200          # thoát: Ctrl-A rồi K
picocom -b 115200 /dev/ttyUSB0      # thoát: Ctrl-A rồi Ctrl-X
```

### Khởi động lên sẽ thấy gì

```raw
App run mode: DEBUG, App version: 0.0.0.3
[task_run] Active Objects is ready
```

Dòng đầu in ở `main_app()`. Dòng sau in ở `task_run()`.

Thấy hai dòng này là kernel đã chạy.

### Lỗi quyền trên Linux

```raw
minicom: cannot open /dev/ttyUSB0: Permission denied
```

Thêm mình vào nhóm `dialout`:

```bash
sudo usermod -a -G dialout $USER
```

Rồi **đăng xuất đăng nhập lại**. Không thì tạm thời:

```bash
sudo chmod 666 /dev/ttyUSB0
```

## 8. Shell trên UART

Kết nối được rồi thì gõ lệnh trực tiếp. Gõ `help` xem có gì.

| Lệnh | Việc |
|---|---|
| `help` | Danh sách lệnh |
| `reboot` / `reset` | Khởi động lại |
| `fatal` | In fatal log: mã lỗi, số lần restart |
| `ram` | Mức dùng RAM và stack |
| `flash` | Thao tác W25Q80 |
| `epprom` / `eps` | Dump hoặc ghi EEPROM nội |
| `lcd` | Test màn hình |
| `beep` | Test buzzer |
| `boot` | Thông tin bootloader |

Bo đã đóng hộp thì đây là công cụ debug mạnh nhất. Code ở `app/shell.cpp`.

## 9. Debug bằng gdb

Cần OpenOCD với ST-Link.

```bash
cd application
make debug
```

Nó mở OpenOCD trong xterm rồi chạy `arm-none-eabi-gdb` với script `stm32l_init.gdb`.

Thích giao diện đồ hoạ thì:

```bash
make debug gdb=ddd
```

Mấy lệnh gdb hay dùng:

```gdb
b main_app              # đặt breakpoint
b dungeon_confirm_action
c                       # chạy tiếp
n                       # bước qua
s                       # bước vào
p dungeon_runtime       # in giá trị struct
bt                      # xem stack trace
```

## 10. Mấy lỗi build hay gặp

### `arm-none-eabi-gcc: No such file or directory`

Chưa cài toolchain, hoặc `GCC_PATH` sai. Kiểm tra:

```bash
which arm-none-eabi-gcc
```

Ra `/usr/bin/arm-none-eabi-gcc` thì `GCC_PATH=/usr` là đúng.

### `No rule to make target ...`

Thường do vừa xoá hay đổi tên file mà `.d` cũ còn trỏ tới. Chữa:

```bash
make clean && make
```

### `region FLASH overflowed by N bytes`

Code quá 116 KB. Cách giảm:

- Tắt `IF_LINK_UART_EN` nếu không cần nạp firmware qua UART
- Tắt bớt cờ log trong `CONSOLE_OPTION`
- Đổi `OPTIMIZE_OPTION` sang `-Os`
- Bỏ bitmap không dùng trong `screens_bitmap.cpp`

### `region SRAM overflowed`

RAM quá 16 KB. Thủ phạm hay gặp:

- Framebuffer OLED 1024 byte, không bỏ được
- PDU pool của link layer 1536 byte, tắt `IF_LINK_UART_EN` là hết
- Mảng toàn cục mới thêm

Mở `.map` ra tìm:

```bash
grep -A3 "^\.bss" build_dungeon-game/dungeon-game.map | head -40
```

### `undefined reference to ...`

File `.cpp` mới chưa được thêm vào `Makefile.mk` của thư mục đó. Ví dụ thêm file trong `app/`:

```makefile
SOURCES_CPP += sources/app/file_moi.cpp
```

### `expected '}' before ...` ở chỗ trông rất bình thường

Coi chừng ký tự lạ do bộ gõ tiếng Việt. Tìm bằng:

```bash
grep -rnP "[^\x00-\x7F]" application/sources/app --include=*.cpp --include=*.h
```

Comment tiếng Việt thì không sao. Nhưng ký tự lạc vào giữa code là lỗi cú pháp.

Lỗi này từng xảy ra thật trong repo này: một ký tự `ư` lạc vào giữa enum `DUNGEON_ACTION_*`.

## 11. Quy trình trước khi nạp

```raw
[ ] make clean && make       -> build sạch
[ ] xem dòng size            -> flash và RAM còn chỗ không
[ ] không có warning mới
[ ] nạp lên bo
[ ] mở make com              -> xem 2 dòng khởi động
[ ] LED life nháy 1 giây     -> kernel còn sống
[ ] bấm 3 nút                -> đúng phản hồi
[ ] chơi hết 1 ván           -> menu, chơi, game over
[ ] reset bo                 -> save game còn không
```
