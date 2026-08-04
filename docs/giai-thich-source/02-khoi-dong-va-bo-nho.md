# 02 — Khởi động và bộ nhớ

## 1. Vector table

`platform/stm32l/system.c:98` nhét bảng vector vào section `.isr_vector`:

```c
__attribute__((section(".isr_vector")))
void (* const isr_vector[])() = {
    ((void (*)())(uint32_t)&_estack),  // [0] giá trị nạp vào SP lúc reset
    reset_handler,                     // [1] chạy đầu tiên
    nmi_handler,
    hard_fault_handler,
    mem_manage_handler,
    bus_fault_handler,
    usage_fault_handler,
    0, 0, 0, 0,
    svc_handler,
    dg_monitor_handler,
    0,
    pendsv_handler,
    systick_handler,                   // [15] nhịp tim 1 ms
    /* ... ngắt ngoại vi ... */
};
```

Linker đặt section này ở đầu vùng FLASH, tức `0x08003000` với application. Xem `ak.ld`.

Bốn handler lỗi đều gọi `FATAL()`:

```c
void hard_fault_handler()  { FATAL("SY", 0x02); }
void mem_manage_handler()  { FATAL("SY", 0x03); }
void bus_fault_handler()   { FATAL("SY", 0x04); }
void usage_fault_handler() { FATAL("SY", 0x05); }
```

Crash là in mã lỗi ra UART, ghi log vào W25Q80, rồi reset. Không đứng im. Code trong `sys/sys_dbg.c`.

## 2. reset_handler

`platform/stm32l/system.c:212`. Đây là hàm C đầu tiên được chạy.

```c
void reset_handler() {
    __disable_irq();                       // 1. cấm ngắt trong lúc dựng môi trường

    SystemInit();                          // 2. cấu hình cơ bản, của ST

    while (pDest < &_edata) *pDest++ = *pSrc++;   // 3. copy .data từ FLASH sang SRAM
    for (pDest = &_bss; pDest < &_ebss; pDest++) *pDest = 0UL;   // 4. xoá .bss

    sys_stack_count_init();                // 5. tô stack bằng mẫu để đo mức dùng

    ENTRY_CRITICAL();
    sys_cfg_clock();                       // 6. clock hệ thống
    sys_cfg_svc();                         // 7. ưu tiên ngắt SVC
    sys_cfg_pendsv();                      // 8. ưu tiên ngắt PendSV
    sys_cfg_tick();                        // 9. SysTick 1 ms  <-- nhịp tim
    sys_cfg_console();                     // 10. UART debug

    cnt = __preinit_array_end - __preinit_array_start;   // 11. constructor C++
    for (i = 0; i < cnt; i++) __preinit_array_start[i]();
    _init();
    cnt = __init_array_end - __init_array_start;
    for (i = 0; i < cnt; i++) __init_array_start[i]();

    sys_ctrl_delay(100);                   // 12. chờ ổn định
    sys_cfg_update_info();                 // 13. đọc thông tin chip
    main_app();                            // 14. sang tầng ứng dụng
}
```

Bước 11 đáng để ý.

Project này viết C++, có biến toàn cục kiểu class. Ví dụ `Adafruit_oled_drv view_render;`. Constructor của mấy cái đó phải chạy trước `main_app()`.

Vòng lặp `__init_array` chính là chỗ làm việc đó. Bình thường `main()` của libc lo hộ, nhưng ở đây không dùng startup của thư viện nên phải tự làm.

## 3. Linker script

`platform/stm32l/ak.ld`:

```ld
MEMORY {
    BSF   (rx)  : ORIGIN = 0x08002000, LENGTH = 4K     /* Boot Share Flash */
    FLASH (rx)  : ORIGIN = 0x08003000, LENGTH = 116K   /* code + rodata */
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}
HEAP_SIZE = 2K;
```

SRAM xếp theo thứ tự khai trong `SECTIONS`:

```raw
0x20000000  ┌──────────────────┐
            │ .data            │  biến toàn cục CÓ giá trị khởi tạo
            │                  │  (copy từ flash ở bước 3)
            ├──────────────────┤
            │ .bss             │  biến toàn cục = 0
            │                  │  (xoá ở bước 4)
            ├──────────────────┤
            │ .heap    2 KB    │  cho new/malloc, dynamic msg pool xài
            ├──────────────────┤
            │ .non_clear_ram   │  KHÔNG bị xoá khi reset mềm
            ├──────────────────┤
            │                  │
            │      stack       │  mọc ngược xuống từ _estack
            │        ▲         │
0x20004000  └────────┴─────────┘  _estack
```

### Vùng .non_clear_ram

```ld
.non_clear_ram (NOLOAD) : {
    _non_clear_ram = .;
    KEEP (*(.non_clear_ram))
    KEEP (*(.non_clear_ram*))
    _enon_clear_ram = .;
} > SRAM
```

Biến đặt vào đây không nằm trong `.data`, cũng không nằm trong `.bss`. Nên `reset_handler` không đụng tới. Reset mềm xong (watchdog đá, hay `sys_ctrl_reset()`) giá trị cũ vẫn còn.

Biến trong đó là gì: `sys_soft_reboot_counter`, đếm số lần khởi động lại. Xem `app/app_non_clear_ram.cpp`.

Trong `main_app()`:

```c
sys_soft_reboot_counter++;
...
if (boot_app_share_data.is_power_on_reset == SYS_POWER_ON_RESET) {
    app_power_on_reset();       // chỉ khi cắm điện mới thì reset bộ đếm về 0
}
```

Nhờ vậy phân biệt được "vừa cắm điện" với "vừa bị watchdog đá".

Về cái `(NOLOAD)`: thiếu nó thì linker vẫn cấp địa chỉ nạp trong ảnh flash, file `.bin` bị đệm thêm, mà ý nghĩa lại mâu thuẫn với tên section. Bản gốc thiếu, đã thêm vào.

## 4. Stack còn bao nhiêu

Build xong mở file `.map` ra xem:

```raw
_end_ram   = <cuối non_clear_ram>
_sstack    = _end_ram
_estack    = 0x20004000
```

Stack khả dụng là `_estack - _sstack`. Lần build gần nhất khoảng 2.3 KB.

Frame nào ăn nhiều nhất, đọc từ file `.su` do `-fstack-usage` sinh ra:

| Hàm | Bytes |
|---|---|
| `fsm_link_state_handle` | 496 |
| `task_fw` | 216 |
| `dtostrf` | 168 |
| `shell_fatal` | 128 |
| `xvfprintf` | 120 |

Còn phải cộng ngắt lồng nhau nữa. 2.3 KB là đủ, nhưng không dư. Thêm hàm nào có buffer to trên stack thì phải kiểm lại ngay.

`sys_stack_count_init()` tô mẫu lên stack lúc khởi động. Gõ lệnh shell `ram` là in ra mức dùng cao nhất thực tế.

## 5. Boot bàn giao cho app

Vùng BSF ở `0x08002000`, 4 KB, là chỗ bootloader với application nói chuyện với nhau.

Cấu trúc `sys_boot_t` trong `sys/sys_boot.h`:

```c
#define FIRMWARE_PSK  0x1A2B3C4D    /* magic number, nhận dạng vùng hợp lệ */

#define SYS_BOOT_CMD_NONE        0x01
#define SYS_BOOT_CMD_UPDATE_REQ  0x02
#define SYS_BOOT_CMD_UPDATE_RES  0x03

#define SYS_BOOT_CONTAINER_DIRECTLY        0x01   /* nhận thẳng qua io driver */
#define SYS_BOOT_CONTAINER_EXTERNAL_FLASH  0x02   /* ảnh nằm sẵn ở W25Q80 */
...
#define SYS_BOOT_IO_DRIVER_UART  0x02
```

Application muốn nạp firmware mới thì ghi lệnh vào đây rồi tự reset:

```c
sys_boot_t sb;
sys_boot_get(&sb);
sb.fw_app_cmd.cmd       = SYS_BOOT_CMD_UPDATE_REQ;
sb.fw_app_cmd.container = SYS_BOOT_CONTAINER_DIRECTLY;
sb.fw_app_cmd.io_driver = SYS_BOOT_IO_DRIVER_UART;
sb.fw_app_cmd.des_addr  = APP_START_ADDR;    /* 0x08003000 */
sb.fw_app_cmd.src_addr  = 0;
sys_boot_set(&sb);
sys_ctrl_reset();
```

Reset xong bootloader đọc BSF, thấy `UPDATE_REQ` thì vào chế độ nhận file thay vì nhảy sang app. Chi tiết ở [06-bootloader.md](06-bootloader.md).

## 6. W25Q80 chứa gì

Xem `app/app_flash.h`:

| Địa chỉ | Nội dung |
|---|---|
| `0x02000` | Fatal log: mã lỗi, số lần khởi động lại |
| `0x03000` | IRQ log |
| `0x04000`, `0x05000` | Log message của kernel, khi bật `AK_TASK_OBJ_LOG_ENABLE` |
| `0x07000` | Boot share data, bản sao ở flash ngoài |
| `0x20000` | Vùng dump RAM khi crash |
| `0x80000` | Ảnh firmware chờ nạp, 2 block 64 KB |

Mỗi lần khởi động `main_app()` đều cộng bộ đếm:

```c
fatal_log_t app_fatal_log;
flash_read(APP_FLASH_AK_DBG_FATAL_LOG_SECTOR, (uint8_t*)&app_fatal_log, sizeof(fatal_log_t));
app_fatal_log.restart_times++;
flash_erase_sector(APP_FLASH_AK_DBG_FATAL_LOG_SECTOR);
flash_write(APP_FLASH_AK_DBG_FATAL_LOG_SECTOR, (uint8_t*)&app_fatal_log, sizeof(fatal_log_t));
```

Xem bằng lệnh shell `fatal`.

## 7. Cấu hình build

Trong `application/Makefile`. Mấy nhóm cờ nên biết:

```makefile
APP_START_ADDR_VAL = 0x08003000       # phải khớp ak.ld

OLED_OPTION += -DSSD1309_DRIVER_EN \  # chọn controller OLED
               -USH1106_DRIVER_EN

CONSOLE_OPTION += -DLOGIN_PRINT_EN \  # bật tắt từng nhóm log
                  -DSYS_PRINT_EN   \
                  -DAPP_PRINT_EN   \
                  -DSYS_DBG_EN     \
                  -DAPP_DBG_EN
#                 -DAPP_DBG_SIG_EN    # đang tắt, in mọi signal làm chậm game

IF_LINK_OPTION = -DIF_LINK_UART_EN    # bật tầng link UART
```

Macro tương ứng ở `app/app_dbg.h`:

```c
#if defined(APP_DBG_SIG_EN)
#define APP_DBG_SIG(fmt, ...)  xprintf("-SIG-> " fmt, ##__VA_ARGS__)
#else
#define APP_DBG_SIG(fmt, ...)          // rỗng, compiler xoá sạch
#endif
```

Tắt là mấy dòng `APP_DBG_SIG(...)` biến mất khỏi binary luôn. Không tốn flash, không tốn thời gian.

Vì sao phải tắt: `xprintf` ghi UART kiểu blocking. Tick 100 ms bắn 4 message cho 5 task, mỗi frame in cả chục dòng, trễ thấy rõ luôn. Cần soi luồng signal thì bật lại.
