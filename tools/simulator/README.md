# Dungeon simulator - chạy game trên máy tính, không cần bo

Chạy **đúng firmware thật** trên macOS hoặc Linux: đúng kernel AK, đúng 6 task
game, đúng logic chiến đấu, đúng bản ghi EEPROM. Chỉ có phần cứng là giả.

Dùng để xem giao diện, thử luồng chơi, hoặc dựng lại một con bug mà không phải
nạp bo mỗi lần sửa một dòng.

---

## Chạy thử

Cần `clang` (macOS: cài Xcode Command Line Tools bằng `xcode-select --install`)
hoặc `g++`. **Không** cần `arm-none-eabi-gcc`, không cần thư viện ngoài.

```bash
cd tools/simulator
make
```

Hai kiểu hiển thị:

```bash
./dungeon-sim          # vẽ thẳng trong Terminal
./dungeon-sim --web    # mở http://localhost:8080, nhìn như cái máy thật
```

Hoặc gõ tắt: `make run` và `make web`.

> Kiểu Terminal cần cửa sổ rộng ít nhất **132 cột**. Hẹp hơn thì hình bị gãy dòng.
> Không muốn chỉnh cửa sổ thì dùng `--web` cho nhanh.

## Phím

| Phím | Tương ứng nút trên bo |
| --- | --- |
| `↑` hoặc `W` | UP |
| `↓` hoặc `S` | DOWN |
| `Enter` / `Space` / `M` | MODE |
| `Shift` + phím trên | bấm giữ (long press) |
| `R` | Reset - dựng lại kernel, EEPROM giữ nguyên |
| `Q` hoặc `Ctrl+C` | thoát |

Ở kiểu `--web` thì bấm phím trên trang, hoặc bấm mấy nút trên màn hình.

## Tham số

| Tham số | Ý nghĩa |
| --- | --- |
| `--web` | Chạy máy chủ web thay vì vẽ trong Terminal |
| `--port N` | Đổi cổng, mặc định 8080 |
| `--wipe` | Xoá sạch EEPROM trước khi chạy: mất save, mất điểm cao, mất setting |
| `--eeprom FILE` | Đổi file EEPROM, mặc định `dungeon-sim.eeprom` |

---

## Thử lại lỗi mất điện

Đây là chỗ bản giả lập ăn đứt việc test trên bo: `kill -9` chính là rút điện,
mà lại làm được đúng vào mili giây mình muốn.

```bash
./dungeon-sim --web --wipe          # ván mới, EEPROM sạch
# chơi tới lúc nào đó, ví dụ đang ở bảng "Monster appears"

# ở cửa sổ Terminal khác:
pkill -9 -x dungeon-sim             # rút điện, không kịp ghi thêm gì

./dungeon-sim --web                 # cắm điện lại
# Menu phải hiện Continue, bấm vào phải quay đúng chỗ đang chơi dở
```

File `dungeon-sim.eeprom` chính là 4 KB EEPROM trong con STM32L151. Muốn xem
bản ghi save nằm ở đâu thì mở bằng hex editor, địa chỉ `0x0140`:

```bash
xxd -s 0x0140 -l 64 dungeon-sim.eeprom
```

Bốn byte đầu phải là `44 55 4e 32` — chữ `DUN2`, magic của bản ghi save.

---

## Giống bo thật tới đâu

**Giống hệt** (dùng thẳng file nguồn trong `application/sources/`, không chép,
không sửa):

- Kernel AK: bảng task, mức ưu tiên, hàng đợi message, bitmap sẵn sàng, timer mềm
- Cả 6 task game và toàn bộ logic chiến đấu
- Toàn bộ 12 màn hình và hệ hằng số bố cục
- `app_eeprom.cpp`: magic, checksum, giá trị mặc định khi hỏng
- Vòng lặp chính: bản giả lập móc vào `task_polling_console()` mà kernel gọi
  sẵn mỗi vòng `task_run()`, nên **không sửa một dòng nào trong `ak/`**

**Giả** (nằm hết trong `sim_platform.cpp` và `stub/`):

| Phần cứng | Bản giả lập làm gì |
| --- | --- |
| EEPROM 4 KB | Mảng trong RAM, ghi ra file sau mỗi `eeprom_write` |
| Màn OLED | Framebuffer 128x64 phẳng, không đẩy I2C |
| Nút bấm | Bàn phím, định tuyến chép y `app_bsp.cpp` |
| Còi | Chỉ đếm số lần kêu, không phát tiếng |
| Watchdog | Không làm gì |
| SysTick | Đồng hồ thật của máy, bù tối đa 50 ms mỗi vòng |
| NOR flash W25Q80 | Chưa có - phần bootloader và fatal log không chạy ở đây |

**Khác biệt cần biết**

- `Adafruit_GFX` trong `stub/` là bản viết lại cho máy tính. Driver thật kéo
  theo cả tầng tương thích Arduino (`Arduino.h` → `sys_ctrl`, `sys_io`,
  `sys_cfg`; `Print.h` → `WString`, `Printable`), chẳng có ý nghĩa gì trên
  laptop. Các hàm vẽ được chép nguyên thuật toán: cùng vòng Bresenham cho hình
  tròn, cùng hàm bo góc, cùng font 5x8, cùng bước nhảy con trỏ `6*size`. Chỉ
  khác chỗ pixel rơi vào: mảng phẳng thay vì bố cục trang của SSD1309.
- Không có bootloader. Bản giả lập vào thẳng `main_app()`.
- Tầng LINK/PHY/MAC để trống.

---

## Cấu trúc

```text
tools/simulator/
  sim.h            khai báo dùng chung
  sim_platform.cpp phần cứng giả: EEPROM, đồng hồ, còi, critical section
  sim_core.cpp     dựng kernel, định tuyến nút, xuất framebuffer
  sim_main.cpp     hai kiểu hiển thị: Terminal và web
  stub/            Adafruit_GFX cho máy tính + mấy header rỗng
  Makefile
```

Thêm màn hình hay task mới vào firmware thì **không phải sửa gì ở đây** —
Makefile quét `application/sources/app/screens/*.cpp` và
`application/sources/app/game/dungeon_game/*.cpp` bằng wildcard.

## Dọn

```bash
make clean
rm -f dungeon-sim.eeprom     # xoá luôn save
```
