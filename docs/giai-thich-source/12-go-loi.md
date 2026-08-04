# 12 — Gỡ lỗi

Tra theo triệu chứng. Mỗi mục có thứ tự kiểm từ dễ tới khó.

## 0. Ba thứ nhìn trước tiên

Trước khi đoán mò, xem ba chỗ này. Chúng trả lời được phần lớn câu hỏi.

### LED life

Nháy 1 giây một lần nghĩa là kernel còn dispatch được message. Nó nháy trong `task_life`, cùng chỗ vỗ watchdog.

| LED | Nghĩa là |
|---|---|
| Nháy đều 1 s | Kernel sống, message chạy bình thường |
| Đứng yên | Có task không chịu return, hoặc kernel chưa chạy |
| Nháy nhanh 250 ms | Đang ở chế độ nạp UART của bootloader |
| Tắt hẳn | Chưa tới `main_app()`, hoặc chết ở `reset_handler` |

### Log UART

```bash
cd application
make com dev=/dev/ttyUSB0
```

Khởi động bình thường sẽ thấy:

```raw
App run mode: DEBUG, App version: 0.0.0.3
[task_run] Active Objects is ready
```

Thiếu dòng đầu là chết trước `main_app()`. Có dòng đầu mà thiếu dòng sau là chết trong lúc khởi tạo phần cứng.

### Fatal log

Gõ `fatal` trên shell. Nó in mã lỗi lần chết gần nhất và số lần khởi động lại.

Mã lỗi dạng hai phần: tên module và số.

| Mã | Nghĩa |
|---|---|
| `SY 0x02` | Hard fault |
| `SY 0x03` | Mem manage fault |
| `SY 0x04` | Bus fault |
| `SY 0x05` | Usage fault |
| `TK 0x01` | `task_create` nhận bảng NULL |
| `TK 0x02` | Post tới task ID không tồn tại |
| `TK 0x05` | `task_remove_msg` với ID sai |
| `TK 0x06` | `task_polling_create` nhận bảng NULL |
| `MF ...` | Hết message pool |
| `SWDG 0x01` | Soft watchdog hết giờ |
| `SCR_MNG 0x01` | Screen manager chưa `SCREEN_CTOR` |
| `LK_PHY ...` | Lỗi tầng PHY của link |

`restart_times` tăng nhanh bất thường nghĩa là bo đang reset vòng lặp.

---

## 1. Màn hình đen thui

### Kiểm theo thứ tự

**LED life có nháy không.**

Không nháy thì vấn đề không nằm ở màn hình, nhảy xuống mục 2.

**Có gọi `view_render.initialize()` chưa.**

Mỗi màn hình phải gọi nó trong `SCREEN_ENTRY`:

```c
case SCREEN_ENTRY:
    view_render.initialize();
    view_render_display_on();
```

Quên một trong hai là màn đen.

**Makefile có chọn controller chưa.**

```bash
cd application
make -n -p 2>/dev/null | grep -o "\-DSSD1309_DRIVER_EN"
```

Không ra gì thì `OLED_OPTION` chưa được đưa vào `GENERAL_FLAGS`. Không chọn controller nào thì driver báo `#error` lúc biên dịch, nhưng chọn sai controller thì biên dịch qua mà màn hình không lên.

**Thử lệnh shell `lcd`.**

Nếu `lcd` vẽ được mà game không vẽ được thì phần cứng ổn, lỗi ở tầng màn hình.

**Xem có `SCREEN_CTOR` chưa.**

`app_task_init()` phải gọi:

```c
SCREEN_CTOR(&scr_mng_app, scr_startup_handle, &scr_startup);
```

Thiếu thì `scr_mng_dispatch` gặp `screen_manager == NULL` rồi `FATAL("SCR_MNG", 0x01)`.

**Kiểm dây.**

Ba chân OLED: `OLED_CLK_PIN` 0x03, `OLED_DATA_PIN` 0x04, `OLED_RES_PIN` 0x08. Mã logic, ánh xạ sang GPIO thật trong `wiring_digital.cpp`.

---

## 2. LED life đứng yên

Nghĩa là `task_life` không được chạy. Kernel kẹt.

### Có task nào không chịu return

Nguyên nhân phổ biến nhất. Tìm trong code mới sửa gần đây:

```bash
grep -rn "while\s*(\|delay\|sys_ctrl_delay" application/sources/app --include=*.cpp
```

Vòng `while` có điều kiện thoát rõ ràng thì được. `while (1)` hay chờ cờ thì chết.

### Kẹt trong ngắt

Callback nút chạy trong ngữ cảnh ngắt. Trong đó chỉ được post message. Gọi hàm nặng, gọi `xprintf`, hay vẽ màn hình là hỏng.

Xem `app/app_bsp.cpp`, mấy hàm `btn_*_callback` chỉ nên có `task_post_pure_msg` với `timer_set`.

### Đệ quy vô tận giữa các task

Task A post cho B, B post lại cho A. Vòng `while` trong `task_sheduler` không bao giờ cạn.

Tìm bằng cách bật log:

```makefile
CONSOLE_OPTION += -DAPP_DBG_SIG_EN
```

Rồi xem UART. Thấy hai signal thay nhau in mãi là ra.

### Tràn stack

Chỉ còn khoảng 2.3 KB. Tràn thì đè lên `.non_clear_ram`, rồi heap, rồi `.bss`. Triệu chứng lung tung, thường là hard fault.

Gõ `ram` trên shell xem mức dùng cao nhất.

Frame lớn nhất hiện có là `fsm_link_state_handle` 496 byte. Thêm hàm nào có mảng lớn trên stack là phải cẩn thận.

---

## 3. Bo reset liên tục

### Xem `restart_times`

Gõ `fatal`. Số này tăng mỗi lần khởi động.

### Watchdog đá

Hai watchdog, hai kiểu chết khác nhau.

`SWDG 0x01` trong fatal log nghĩa là **soft watchdog**. Có nghĩa `task_life` không được chạy trong 20 giây. Quay lại mục 2.

Không có gì trong fatal log mà vẫn reset thì thường là **IWDG** (khoảng 30 giây). Nó reset im lặng, không kịp ghi log. Cũng cùng nguyên nhân: kernel kẹt.

### Hard fault

`SY 0x02` trong fatal log.

Hay gặp nhất:

- Con trỏ NULL. Nhất là bitmap chưa khai mà đã vẽ.
- Đọc ngoài mảng. Thêm quái mà quên sửa kích thước `dungeon_monster_name[]` là dính.
- Tràn stack.

Truy bằng gdb:

```bash
make debug
```

```gdb
b hard_fault_handler
c
bt
```

### Chết ngay lúc khởi động

Chưa thấy dòng `App run mode` nào cả.

Kiểm `reset_handler` trong `platform/stm32l/system.c:212`. Hay gặp là biến toàn cục C++ có constructor gọi thứ chưa khởi tạo.

Nhớ là `__init_array` chạy **trước** `main_app()`. Constructor mà đụng vào phần cứng chưa cấu hình là chết.

---

## 4. Game đơ, hình giật

### Đo trước, đoán sau

Bật log thời gian của kernel:

```makefile
CONSOLE_OPTION += -DAK_TASK_LOG_CONSOLE_ENABLE
```

Mỗi message xử lý xong sẽ in ra:

```raw
taskID: 6  msgType:0x40  refCnt:1  sig:100  waitTime:2  exeTime:15
```

`waitTime` là thời gian nằm chờ trong hàng đợi. `exeTime` là thời gian chạy thật.

`waitTime` lớn nghĩa là có thằng khác chiếm. `exeTime` lớn nghĩa là chính nó chậm.

### Nghi can quen mặt

**`APP_DBG_SIG_EN` đang bật.** Mỗi signal in một dòng UART theo kiểu blocking. Tick 100 ms bắn 4 message là mỗi frame in cả chục dòng. Tắt đi.

**Vẽ quá nhiều.** Mỗi lần vẽ là 1024 byte qua I2C bit-bang. Kiểm `AC_DISPLAY_MINIMUM_SCREEN_RENDER_INTERVAL_MS` còn 50 không.

**Timer bị đặt trùng.** Vào màn hình mà quên `timer_remove_attr` timer cũ thì hai timer cùng bắn.

Tìm cặp `timer_set` với `timer_remove_attr` xem có khớp không:

```bash
grep -rn "timer_set\|timer_remove_attr" application/sources/app/screens
```

**Logic bị chạy hai lần.** Chuyện này từng xảy ra thật trong repo: `dungeon_tick()` được gọi cả trực tiếp lẫn qua message, thành ra game chạy tốc độ gấp đôi.

Tìm bằng cách grep tên hàm, xem có chỗ nào vừa gọi trực tiếp vừa post message không.

---

## 5. Nút bấm không ăn

### Kiểm theo thứ tự

**Có `button_enable()` chưa.**

`main_app()` phải có đủ:

```c
button_init(&btn_mode, 10, BUTTON_MODE_ID, ...);
button_enable(&btn_mode);
```

Chỉ `button_init` mà quên `button_enable` là nút chết.

**Callback có post message không.**

Xem `app/app_bsp.cpp`. Trong đó có mấy dòng bị comment sẵn:

```c
// task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_BUTTON_MODE_PRESSED);
```

Cố ý đấy. Game chỉ dùng sự kiện `RELEASED`, không dùng `PRESSED`. Nhưng nếu em cần `PRESSED` thì phải bỏ comment.

**Màn hình có xử lý signal đó không.**

Signal tới nhưng màn hình không có `case` cho nó thì rơi vào `default:` rồi bị bỏ qua lặng lẽ.

Bật `-DAPP_DBG_SIG_EN` xem signal có tới không. Tới mà không phản ứng là thiếu `case`.

**Ngắt 10 ms có chạy không.**

`sys_irq_timer_10ms()` trong `app/app.cpp:280` phải gọi `button_timer_polling()` cho cả 3 nút. Thiếu nút nào thì nút đó chết.

**Thời gian nhấn.**

`BUTTON_SHORT_PRESS_MIN_TIME` là 20 ms. Bấm nhanh quá thì bị lọc mất, tưởng là rung phím.

---

## 6. Mất save, điểm về 0

### Sau khi đổi struct

Đổi `dungeon_game_save_t`, hay đổi kích thước `inventory[]`, hay thêm bớt field, là save cũ không đọc được nữa. Kích thước struct đổi thì checksum sai, rơi về mặc định.

Đây là **hành vi đúng**, không phải lỗi. Nhưng cần biết trước.

### Sau khi đổi layout EEPROM

Bản ghi giờ có dạng `[magic][payload][checksum]`. Ai quen bản cũ ghi thẳng struct thì lần đầu chạy sẽ mất hết.

Kiểm địa chỉ có đè nhau không, xem `app/app_eeprom.h`:

```raw
0x0100  setting      12 B  -> hết ở 0x010C
0x0120  best score   20 B  -> hết ở 0x0134
0x0140  save game    52 B  -> hết ở 0x0174
0x0200  điểm ván     9 B   -> hết ở 0x0209
```

Thêm field vào struct mà không dời địa chỉ là bản ghi này đè bản ghi kia.

### Dump EEPROM ra xem

```raw
eps
```

Toàn `FF` nghĩa là chưa ghi gì. Có dữ liệu mà đọc lên vẫn mặc định thì checksum sai.

### Đang ở chế độ creator

`dungeon_persist_enabled = 0` thì cố tình không ghi. Kiểm bằng `dungeon_is_creator_mode()`.

---

## 7. Âm thanh không kêu

**Setting đang im lặng.** `settingdata.silent` bằng 1 thì `BUZZER_Sleep(1)` được gọi lúc khởi động, mọi `BUZZER_PlayTones` đều bị bỏ qua.

**Đang phát bài khác.** Nhìn lại `BUZZER_PlayTones`:

```c
if (_tones == NULL) {      /* chỉ nhận bài mới khi đang rảnh */
    _tones = tones;
    ...
}
```

Bài đang phát thì lệnh mới bị bỏ. Không có hàng đợi nhạc.

**Ngắt TIM3 không chạy.** Nhạc chuyển nốt bằng ngắt. `buzzer_irq()` không được gọi thì nốt đầu kêu mãi hoặc tịt hẳn.

**Thử `beep` trên shell.** Kêu thì phần cứng ổn, lỗi ở tầng game.

---

## 8. Build được mà nạp lên không chạy

**Nạp nhầm địa chỉ.** Application ở `0x08003000`. Nạp vào `0x08000000` là đè bootloader.

Nạp nhầm rồi thì phải nạp lại bootloader bằng ST-Link:

```bash
cd boot && make && make flash
```

**Bootloader không thấy app hợp lệ.** Nó kiểm:

```c
if (fw_app_cmd.cmd == SYS_BOOT_CMD_NONE &&
    current_fw_app_header.psk == FIRMWARE_PSK) {
```

Thiếu magic là không nhảy. Gõ `boot` trên shell xem thông tin BSF.

**Bo đang kẹt ở chế độ nạp.** LED nháy 250 ms. Nút MODE có thể đang bị dính hoặc chập.

---

## 9. Mấy công cụ nên biết

### Xem cái gì chiếm bộ nhớ

```bash
cd application
arm-none-eabi-size build_dungeon-game/dungeon-game.axf
```

Chi tiết hơn thì mở `.map`:

```bash
grep -n "^\.text\|^\.data\|^\.bss" build_dungeon-game/dungeon-game.map
```

Tìm biến to nhất trong `.bss`:

```bash
grep -A200 "^\.bss" build_dungeon-game/dungeon-game.map | sort -k2 -r | head -20
```

### Xem hàm nào ăn stack nhiều

```bash
cat build_dungeon-game/*.su | awk -F'\t' '{print $2, $1}' | sort -rn | head -20
```

### Đọc assembly của một hàm

```bash
make asm
grep -A50 "<dungeon_tick>:" build_dungeon-game/dungeon-game.asm
```

### Tìm ký tự lạ do bộ gõ tiếng Việt

```bash
grep -rnP "[^\x00-\x7F]" application/sources/app --include=*.cpp --include=*.h
```

Trong comment thì không sao. Lạc vào giữa code là lỗi cú pháp, mà thông báo lỗi của compiler lại chỉ vào dòng khác, rất khó tìm.

### Kiểm Makefile hiểu đúng chưa

```bash
make -n -p 2>/dev/null | grep "^GENERAL_FLAGS"
```

In ra toàn bộ cờ biên dịch. Xem `-DSSD1309_DRIVER_EN` có đó không, `-DAPP_DBG_SIG_EN` có bị bật nhầm không.

---

## 10. Bảng tra nhanh

| Triệu chứng | Xem trước |
|---|---|
| Màn đen | `view_render.initialize()`, `SCREEN_CTOR`, define OLED |
| LED đứng | Có `while`/`delay` trong task không |
| Reset vòng lặp | `fatal`, tìm mã `SWDG` hay `SY` |
| Game giật | Tắt `APP_DBG_SIG_EN`, kiểm cap FPS |
| Nút chết | `button_enable`, callback có post không, màn có `case` không |
| Mất save | Struct có đổi không, địa chỉ EEPROM có đè nhau không |
| Không kêu | `settingdata.silent`, thử lệnh `beep` |
| Nạp xong đơ | Địa chỉ `0x08003000`, magic của app |
| Build lỗi lạ | `make clean`, tìm ký tự non-ASCII |
