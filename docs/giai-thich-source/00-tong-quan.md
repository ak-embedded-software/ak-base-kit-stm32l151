# 00 — Tổng quan

Bộ này giải thích source của DUNGEON-GAME, chạy trên AK Embedded Base Kit dùng STM32L151.

Đọc theo thứ tự số. Chỗ nào cũng ghi rõ file với dòng, mở ra đối chiếu được.

## 1. Phần cứng

| Thành phần | Chi tiết | Dùng làm gì |
|---|---|---|
| MCU | STM32L151 (Cortex-M3), 128 KB flash, 16 KB SRAM, 4 KB EEPROM nội | Chạy tất cả |
| Màn hình | OLED 128×64, driver SSD1309 | Hiển thị game |
| NOR flash | W25Q80 (1 MB), qua SPI | Log lỗi, ảnh firmware, dump RAM |
| Nút | 4 nút: Reset (cứng), Up, Down, Mode | Điều khiển |
| Buzzer | PWM qua TIM3 | Âm thanh |
| LED | 1 LED "life" | Nháy 1 giây, báo còn sống |

Có một chỗ rất dễ nhầm. Trong code có hai bộ nhớ để lưu dữ liệu, tên gọi na ná nhau.

`eeprom_read()` với `eeprom_write()` đụng vào **EEPROM nội của con STM32L151**. 4 KB, nằm ở `0x08080000`. Code ở `platform/stm32l/io_cfg.c:357`, hằng số ở `io_cfg.h:213`. Setting, high-score, save game đều nằm đây.

`flash_read()` với `flash_write()` mới là **W25Q80 gắn ngoài** qua SPI. Chỗ này chứa fatal log, boot share data, ảnh firmware. Xem `app/app_flash.h`.

Tóm lại: W25Q80 không dính gì tới điểm số. Điểm nằm ở EEPROM nội.

## 2. Flash chia thế nào

```raw
0x08000000  ┌────────────────────────┐
            │  BOOTLOADER      8 KB  │  boot/
0x08002000  ├────────────────────────┤
            │  BSF             4 KB  │  vùng bắt tay boot <-> app
0x08003000  ├────────────────────────┤
            │  APPLICATION   116 KB  │  application/
0x08020000  └────────────────────────┘

0x08080000  ┌────────────────────────┐
            │  EEPROM nội      4 KB  │  setting / score / save game
0x08081000  └────────────────────────┘

0x20000000  ┌────────────────────────┐
            │  SRAM           16 KB  │  .data + .bss + heap 2K + non_clear_ram + stack
0x20004000  └────────────────────────┘
```

Lấy từ `application/sources/platform/stm32l/ak.ld` và `boot/sources/platform/stm32l/ak.ld`. Khớp với `APP_START_ADDR_VAL = 0x08003000` trong `application/Makefile:21`.

## 3. Ba tầng

```raw
┌──────────────────────────────────────────────────────────┐
│  APPLICATION                                             │
│  screens/ (hiển thị)   game/dungeon_game/ (luật chơi)    │
│  task_display, task_if, task_uart_if, task_life,         │
│  task_fw, task_shell                                     │
├──────────────────────────────────────────────────────────┤
│  AK KERNEL  (ak/)                                        │
│  task.c   - scheduler ưu tiên, chạy hết mới nhả          │
│  message.c- 3 pool message (pure / common / dynamic)     │
│  timer.c  - software timer                               │
│  fsm.c    - máy trạng thái phẳng (link layer dùng)       │
│  tsm.c    - máy trạng thái dạng bảng                     │
├──────────────────────────────────────────────────────────┤
│  PLATFORM + DRIVER                                       │
│  system.c (vector table, systick), sys_cfg.c, io_cfg.c   │
│  driver/: OLED, button, buzzer, flash, eeprom, led, gpio │
│  Libraries/: CMSIS + STM32L1xx StdPeriph (của ST)        │
└──────────────────────────────────────────────────────────┘
```

## 4. Từ lúc cắm điện tới lúc thấy game

```raw
 Cắm điện / Reset
      │
      ▼
 [0x08000000] Bootloader chạy trước
      │  đọc sys_boot_t ở BSF (0x08002000)
      │  - có lệnh update?  -> nhận firmware qua UART, ghi vào 0x08003000
      │  - không có         -> chuyển sang app
      ▼
 [0x08003000] reset_handler()          platform/stm32l/system.c:212
      │  SystemInit()
      │  copy .data từ flash sang SRAM
      │  xoá .bss
      │  sys_cfg_clock()   - clock
      │  sys_cfg_tick()    - SysTick 1 ms
      │  sys_cfg_console() - UART debug
      │  gọi constructor của biến toàn cục C++ (__init_array)
      ▼
 main_app()                            app/app.cpp:93
      │  task_init()          - dựng pool message + timer
      │  task_create()        - nạp bảng task
      │  watchdog 32 s + soft watchdog 20 s
      │  SPI.begin(), ADC, flash SPI
      │  button_init() × 3
      │  BUZZER_Init()
      │  đọc boot share data từ W25Q80
      │  tăng bộ đếm số lần khởi động
      │  app_start_timer()    - hẹn giờ cho task life / fw / display
      │  app_task_init()      - SCREEN_CTOR(scr_startup)
      ▼
 task_run()                            ak/src/task.c:281
      │
      │  for (;;) {
      │      task_sheduler();     <- chạy hết message đang chờ, theo ưu tiên
      │      task_polling_run();  <- lúc rảnh thì đọc UART
      │  }
      ▼
   Vòng lặp vô tận. Từ đây message điều khiển tất cả.
```

## 5. Cái gì đánh thức hệ thống

Chỉ có một thứ thôi: ngắt SysTick 1 ms. Ở `platform/stm32l/system.c:321`.

```c
void systick_handler() {
    task_entry_interrupt();
    millis_current++;          // đồng hồ ms toàn cục
    timer_tick(1);             // báo timer manager là 1 ms đã trôi
    if (div_counter == 0)
        sys_irq_timer_10ms();  // 10 ms một lần: quét 3 nút
    div_counter++;
    task_exit_interrupt();
}
```

Mọi thứ còn lại đều mọc ra từ đây.

`timer_tick()` post `TIMER_TICK` cho `task_timer_tick`. Task đó bắn signal cho các task đã hẹn giờ.

`sys_irq_timer_10ms()` gọi `button_timer_polling()`. Nút đủ điều kiện thì callback trong `app_bsp.cpp` chạy, rồi post signal nút cho `AC_TASK_DISPLAY_ID`.

Không có preemptive, không có nhiều stack. Tất cả task xài chung một stack, chạy tới khi xong thì thôi. Chi tiết ở [01-kernel-ak.md](01-kernel-ak.md).

## 6. Thư mục có gì

```raw
application/
  Makefile                    cấu hình build, chọn driver OLED, bật tắt log
  sources/
    ak/                       KERNEL - đọc file 01
      inc/{ak,task,message,timer,fsm,tsm,port}.h
      src/{task,message,timer,fsm,tsm}.c
    app/                      TẦNG ỨNG DỤNG - đọc file 04
      app.cpp                 main_app(), init phần cứng, ngắt 10 ms
      app_bsp.cpp             callback 3 nút -> post signal
      app_eeprom.cpp/.h       đọc ghi EEPROM có magic + checksum
      app_flash.h             bản đồ sector trên W25Q80
      task_list.cpp/.h        BẢNG TASK - chỗ quan trọng nhất
      task_display.cpp        vỏ mỏng, đẩy message vào screen manager
      task_if / task_uart_if  đưa message ra vào UART
      task_life.cpp           nháy LED + vỗ watchdog
      task_fw.cpp             máy trạng thái cập nhật firmware
      task_shell.cpp          shell UART
      shell.cpp               lệnh help/reboot/fatal/ram/flash/epprom/lcd/beep/boot
      screens/                MÀN HÌNH - đọc file 05
      game/dungeon_game/      LUẬT CHƠI - đọc file 05
    common/                   screen_manager, view_render, view_item,
                              fifo, ring_buffer, log_queue, xprintf, cmd_line
    driver/                   OLED, button, buzzer, flash, eeprom, led, gpio
    networks/net/link/        giao thức UART 3 tầng - đọc file 07
    platform/stm32l/          PHẦN CỨNG - đọc file 03
    sys/                      sys_boot, sys_dbg, sys_ctrl, sys_irq
boot/                         BOOTLOADER - đọc file 06
docs/                         tài liệu
hardware/                     schematic, BOM, gerber
resources/                    ảnh bitmap gốc
```

## 7. Đọc tiếp

| File | Nội dung |
|---|---|
| [01-kernel-ak.md](01-kernel-ak.md) | Scheduler, message pool, timer, FSM/TSM |
| [02-khoi-dong-va-bo-nho.md](02-khoi-dong-va-bo-nho.md) | Vector table, linker script, .data/.bss, stack, non_clear_ram |
| [03-platform-va-driver.md](03-platform-va-driver.md) | Clock, SysTick, watchdog, OLED, nút, buzzer, flash, EEPROM |
| [04-tang-application.md](04-tang-application.md) | Bảng task, screen manager, view render, các task hệ thống |
| [05-game-dungeon.md](05-game-dungeon.md) | 5 task game, state chung, luồng 1 tick, luồng 1 lượt đánh |
| [06-bootloader.md](06-bootloader.md) | Bắt tay boot với app, cập nhật firmware |
| [07-networks-link.md](07-networks-link.md) | Giao thức UART 3 tầng PHY/MAC/LINK |
| [08-bao-ve-rtos.md](08-bao-ve-rtos.md) | RTOS nằm ở đâu, với mấy câu giáo viên hay hỏi |

Bốn file thực hành, đọc khi bắt tay làm:

| File | Nội dung |
|---|---|
| [09-dung-moi-truong-va-nap-bo.md](09-dung-moi-truong-va-nap-bo.md) | Cài toolchain, build, nạp bo, xem log UART, lỗi build hay gặp |
| [10-huong-dan-mo-rong.md](10-huong-dan-mo-rong.md) | Công thức thêm task, màn hình, quái, item, lệnh shell |
| [11-mo-xe-tung-dong.md](11-mo-xe-tung-dong.md) | Đọc từng dòng 5 hàm cốt lõi |
| [12-go-loi.md](12-go-loi.md) | Tra theo triệu chứng khi hỏng |

Chưa từng đụng project thì đọc [09](09-dung-moi-truong-va-nap-bo.md) trước, build cho chạy đã, rồi mới quay lại 01.
