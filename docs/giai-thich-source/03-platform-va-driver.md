# 03 — Platform và driver

## 1. Clock

Ở `platform/stm32l/sys_cfg.c:43`. Dùng HSI, tức bộ dao động nội 16 MHz. Không có thạch anh ngoài.

```c
RCC_HSICmd(ENABLE);
while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);
SystemCoreClockUpdate();
```

Được cái đỡ linh kiện, khởi động nhanh. Mất cái sai số tần số lớn hơn thạch anh. Với game với UART 115200 thì không sao.

## 2. SysTick 1 ms

```c
volatile uint32_t ticks = SystemCoreClock / 1000;   // sys_cfg.c:54
```

Handler ở `system.c:321`, đã nói ở [01](01-kernel-ak.md#5-software-timer). Làm 3 việc:

1. `millis_current++`, đây là nguồn của `sys_ctrl_millis()`
2. `timer_tick(1)`, nuôi software timer của AK
3. Cứ 10 ms thì gọi `sys_irq_timer_10ms()` để quét nút

## 3. Hai lớp watchdog

Chỗ này đáng khoe lúc bảo vệ. Hệ thống có hai tầng chống treo, mục đích khác nhau.

### Independent watchdog, phần cứng

`sys_cfg.c:415`:

```c
IWDG_SetPrescaler(IWDG_Prescaler_256);   // 37 kHz (LSI) / 256 = 0.144 kHz
IWDG_SetReload(0xFFF);                   // khoảng 30 s
IWDG_Enable();
```

Nó chạy bằng dao động LSI riêng, không dính clock chính. CPU có loạn clock nó vẫn đếm. Bật rồi thì không tắt được.

### Soft watchdog, dùng TIM7

Cũng trong `sys_cfg.c`. TIM7 ngắt định kỳ, mỗi lần gọi:

```c
void sys_ctrl_soft_watchdog_increase_counter() {
    sys_ctrl_soft_counter++;
    if (sys_ctrl_soft_counter >= sys_ctrl_soft_time_out) {
        TIM_Cmd(TIM7, DISABLE);
        FATAL("SWDG", 0x01);      // in log, ghi flash, rồi reset
    }
}
```

Khác nhau ở đây. IWDG reset im lặng, chết mà không biết vì sao. Soft watchdog gọi `FATAL()` nên ghi được log trước khi chết, biết chết ở đâu.

### Ai vỗ chúng

`app/task_life.cpp:27`. Task life, hẹn giờ 1000 ms lặp:

```c
case AC_LIFE_SYSTEM_CHECK:
    sys_ctrl_independent_watchdog_reset();
    sys_ctrl_soft_watchdog_reset();
    led_toggle(&led_life);
    ...
```

Cho nên LED nháy đồng nghĩa với kernel còn dispatch được message. LED đứng yên là có task nào đó không chịu return.

Đây là cách chẩn đoán rẻ nhất trên bo.

Khởi tạo trong `main_app()`:

```c
sys_ctrl_independent_watchdog_init();   /* 32s */
sys_ctrl_soft_watchdog_init(200);       /* 20s */
```

## 4. Chân nào nối gì

Xem `platform/stm32l/io_cfg.h`:

| Chức năng | Chân |
|---|---|
| Nút DOWN | PB3 |
| Nút UP | PC13 |
| Nút MODE | PB4 |
| LED life | PB8 |
| Buzzer | PB0, alternate function TIM3 |
| Flash W25Q80 CE | PB14, cộng SPI |
| OLED CLK / DATA / RES | mã pin logic 0x03 / 0x04 / 0x08 |

Ba chân OLED không dùng `GPIO_Pin_x` mà dùng mã pin logic. Chúng đi qua lớp Arduino ở `platform/stm32l/arduino/cores/wiring_digital.cpp`, trong đó có `switch` ánh xạ mã logic sang GPIO thật.

```c
#define SSD1306_CLK_PIN   (0x03)
#define SSD1306_DATA_PIN  (0x04)
#define SSD1306_RES_PIN   (0x08)

/* Adafruit_oled_drv là driver chung cho SH1106/SSD1306/SSD1309,
 * nó hỏi tên OLED_*_PIN. Cùng 3 chân, chỉ khác tên. */
#define OLED_CLK_PIN      SSD1306_CLK_PIN
#define OLED_DATA_PIN     SSD1306_DATA_PIN
#define OLED_RES_PIN      SSD1306_RES_PIN
```

## 5. Driver OLED

Thư mục `driver/Adafruit_oled_drv/`, có 3 file:

| File | Làm gì |
|---|---|
| `Adafruit_GFX.cpp` | Vẽ hình học thuần phần mềm: line, rect, circle, triangle, bitmap, chữ |
| `Adafruit_oled_drv.cpp` | Lớp dưới: bit-bang I2C, gửi lệnh, đẩy framebuffer |
| `glcdfont.cpp` | Font 5×7 |

### Framebuffer

```c
#define WIDTH  128
#define HEIGHT 64
#define FBSIZE 1024      // 128 × 64 / 8
#define MAXROW 8         // 8 page, mỗi page cao 8 pixel
```

1024 byte nằm trong SRAM 16 KB, tức 6 %.

Mọi lệnh vẽ chỉ sửa RAM này thôi. Phải gọi `update()` mới thực sự đẩy ra màn hình.

Đó là lý do việc giới hạn tần suất `update()` lại quan trọng. Xem [04](04-tang-application.md#3-screen-manager).

### Chọn controller lúc biên dịch

```c
#if defined(SH1106_DRIVER_EN)
    ...chuỗi lệnh init SH1106...
#elif defined(SSD1306_DRIVER_EN)
    ...
#elif defined(SSD1309_DRIVER_EN)
    writeCommand(SSD1309_DISPLAY_OFF);
    writeCommand(SSD1309_MEMORY_MODE);
    writeCommand(SSD1309_SET_DISPLAY_START_LINE_BASE);
    writeCommand(SSD1309_SEG_REMAP_FLIP);
    writeCommand(SSD1309_COM_SCAN_DEC);
    ... contrast, precharge, VCOM, charge pump ...
    writeCommand(SSD1309_DISPLAY_ON);
#else
    #error "Don't know oled driver type."
#endif
```

Bo AK v3 dùng SSD1309, nên Makefile đặt `-DSSD1309_DRIVER_EN`.

Cái `#error` ở nhánh cuối hay đấy. Quên chọn thì lỗi ngay lúc biên dịch, chứ không phải màn hình trắng lúc chạy rồi ngồi mò.

### Đẩy khung hình

```c
void Adafruit_oled_drv::update() {
    unsigned int fbIndex = 0;
    for (unsigned int page = 0; page < MAXROW; page++) {
        writeCommand(0xB0 + page);   // chọn page
        writeCommand(0x00);          // cột thấp = 0
        writeCommand(0x10);          // cột cao  = 0
        startDataSequence();
        for (unsigned int col = 0; col < WIDTH; col++)
            writeByte(m_pFramebuffer[fbIndex++]);
        stopIIC();
    }
}
```

I2C ở đây là bit-bang, tự nhấp chân bằng phần mềm, không dùng ngoại vi I2C của chip.

Nghĩa là đẩy 1024 byte thì chiếm CPU suốt thời gian đó, và task gọi nó bị chặn.

Có hàm `updateRow()` để đẩy một phần thôi, nhưng game hiện chưa dùng.

## 6. Driver nút

`driver/button/button.h` với `.c`. Máy trạng thái 3 mức:

```c
#define BUTTON_SHORT_PRESS_MIN_TIME  (20)     /* 20 ms, lọc rung phím */
#define BUTTON_LONG_PRESS_TIME       (2000)   /* 2 s, nhấn giữ */

#define BUTTON_SW_STATE_PRESSED       (0x00)
#define BUTTON_SW_STATE_LONG_PRESSED  (0x01)
#define BUTTON_SW_STATE_RELEASED      (0x02)
```

Khởi tạo trong `main_app()`. Kiểu dependency injection, driver không cần biết GPIO nào:

```c
button_init(&btn_mode, 10, BUTTON_MODE_ID,
            io_button_mode_init,   // hàm cấu hình chân
            io_button_mode_read,   // hàm đọc mức logic
            btn_mode_callback);    // gọi khi có sự kiện
```

Số 10 là chu kỳ quét tính bằng ms, khớp với `sys_irq_timer_10ms()`.

Bấm một cái nút thì đi qua chừng này chặng:

```raw
SysTick 1 ms ──(mỗi 10 lần)──> sys_irq_timer_10ms()        app/app.cpp:280
                                    │
                                    ▼
                          button_timer_polling(&btn_mode)   driver/button/button.c
                                    │  đọc chân, lọc rung, đếm thời gian giữ
                                    ▼
                          btn_mode_callback(state)          app/app_bsp.cpp
                                    │
                                    ▼
                  task_post_pure_msg(AC_TASK_DISPLAY_ID,
                                     AC_DISPLAY_BUTTON_MODE_RELEASED)
                                    │
                                    ▼
                          (thoát ngắt, kernel dispatch)
                                    ▼
                          task_display -> scr_mng_dispatch -> màn hình hiện tại
```

Điểm mấu chốt: callback chạy trong ngữ cảnh ngắt. Nên nó chỉ được post message rồi thoát. Việc nặng để task làm. Ngắt càng ngắn càng tốt.

## 7. Driver buzzer

`driver/buzzer/buzzer.c`. PWM bằng TIM3, không chặn:

```c
void BUZZER_PlayTones(const Tone_TypeDef * tones) {
    if (_buzzer_sleep == 0) {
        if (_tones == NULL) {              // đang rảnh mới nhận bài mới
            _tones = tones;
            _tones_playing = true;
            BUZZER_Enable(_tones->frequency, _tones->duration);
        }
    }
}
```

Rồi ngắt TIM3 tự chuyển sang nốt kế:

```c
void buzzer_irq(void) {
    if (BUZZER_TIM->SR & TIM_SR_UIF) {
        BUZZER_TIM->SR &= ~TIM_SR_UIF;
        _beep_duration--;
        if (_beep_duration == 0) {
            if (_tones_playing) {
                _tones++;                                  // nốt tiếp theo
                if (_tones->frequency == 0 && _tones->duration == 0) {
                    BUZZER_Disable();                      // hết bài
                    _tones_playing = false;
                    _tones = NULL;
                } else if (_tones->frequency == 0) {
                    ...khoảng lặng...
                } else {
                    BUZZER_Enable(_tones->frequency, _tones->duration);
                }
            } else BUZZER_Disable();
        }
    }
}
```

Gọi `BUZZER_PlayTones()` xong là return liền. Nhạc phát nền bằng ngắt. Đúng kiểu không chặn của AK.

Tiết kiệm điện thì `BUZZER_Disable()` tắt clock TIM3 và đưa chân về analog input.

### Bảng nhạc

`driver/buzzer/buzzer_music.h` với `.c`. Có 9 bài: `tones_cc`, `tones_startup`, `tones_3beep`, `tones_BUM`, `tones_USB_con/dis`, `tones_Lets_go`, `tones_SMB` (Super Mario), `tones_merryChristmas`.

Định dạng là mảng `{tần số Hz, thời lượng}`, kết thúc bằng `{0, 0}`. `{0, n}` nghĩa là im lặng n đơn vị.

Bảng nhạc phải để ở `.c`, không được để ở header. Khai `static const` trong `buzzer.h` thì 14 file include nó sẽ sinh 14 bản sao trong flash. Bản gốc bị lỗi này, đã tách ra rồi.

## 8. EEPROM với W25Q80, đừng nhầm

### EEPROM nội, 4 KB

`driver/eeprom/eeprom.cpp` gọi xuống `platform/stm32l/io_cfg.c:357`:

```c
#define EEPROM_BASE_ADDRESS  (0x08080000)
#define EEPROM_MAX_SIZE      (0x1000)          // 4 KB

uint8_t io_eeprom_read(uint32_t address, uint8_t* pbuf, uint32_t len) {
    uint32_t eeprom_address = address + EEPROM_BASE_ADDRESS;
    if (pbuf == 0 || len == 0 || (address + len) > EEPROM_MAX_SIZE)
        return EEPROM_DRIVER_NG;
    DATA_EEPROM_Unlock();
    memcpy(pbuf, (const uint8_t*)eeprom_address, len);
    DATA_EEPROM_Lock();
    return EEPROM_DRIVER_OK;
}
```

Đọc bằng `memcpy` được vì EEPROM của STM32L nằm trong không gian địa chỉ, đọc như RAM luôn. Ghi thì mới cần unlock.

Đặc điểm: ghi được từng byte, không cần xoá sector trước, chịu được khoảng 10⁵ lần ghi. Rất hợp để lưu setting với điểm.

### W25Q80, 1 MB, qua SPI

`driver/flash/flash.c`. NOR flash thật, phải xoá cả sector rồi mới ghi được. Thấy rõ trong `main_app()`:

```c
flash_erase_sector(APP_FLASH_AK_DBG_FATAL_LOG_SECTOR);
flash_write(APP_FLASH_AK_DBG_FATAL_LOG_SECTOR, ...);
```

Dùng cho fatal log, IRQ log, boot share data, dump RAM, ảnh firmware. Bảng đầy đủ ở [02](02-khoi-dong-va-bo-nho.md#6-w25q80-chứa-gì).

### Lớp kiểm tra ở trên

`app/app_eeprom.cpp` bọc EEPROM nội lại. Mỗi bản ghi có dạng:

```raw
[magic 4 byte][payload][checksum 1 byte]
```

```c
static bool dungeon_eeprom_is_valid(uint32_t* magic, uint8_t sum, uint32_t size) {
    return (*magic == DUNGEON_EEPROM_MAGIC) &&
           (sum == dungeon_eeprom_checksum((uint8_t*)magic, size));
}
```

Đọc hỏng thì trả giá trị mặc định và `false`.

Không có lớp này thì lần chạy đầu tiên, EEPROM còn toàn `0xFF`, đọc lên thành điểm số với setting rác.

Bản đồ địa chỉ, xem `app/app_eeprom.h`:

| Địa chỉ | Bản ghi | Kích thước |
|---|---|---|
| `0x0100` | setting | 12 B |
| `0x0120` | best score | 20 B |
| `0x0140` | save game | 52 B, có magic riêng |
| `0x0200` | điểm ván vừa rồi | 9 B |

## 9. Ưu tiên ngắt

`sys/sys_irq.h` định nghĩa thứ tự ưu tiên ngắt. `IRQ_PRIO_TIMER7_SOFT_WATCHDOG` để cao nhất, vì soft watchdog phải bắt được kể cả khi ngắt khác đang bận.

`task_entry_interrupt()` với `task_exit_interrupt()` gọi ở đầu và cuối `systick_handler`. Chúng báo cho kernel biết đang ở trong ngắt, để `task_self()` trả về `AK_TASK_INTERRUPT_ID` thay vì ID task.
