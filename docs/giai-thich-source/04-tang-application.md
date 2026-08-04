# 04 — Tầng application

## 1. Không gian signal

Mỗi task có không gian signal riêng của nó. Signal số 1 của task display khác hoàn toàn signal số 1 của task fw.

Kernel chỉ định tuyến theo `des_task_id`. Còn `sig` thì task tự hiểu.

`ak/inc/ak.h` chia làm 3 vùng:

```c
#define AK_SYS_DEFINE_SIG   (0)     // signal nội bộ của kernel/khung
#define AK_USER_DEFINE_SIG  (10)    // signal cấp ứng dụng
#define DUNGEON_DEFINE_SIG  (100)   // signal riêng của game
```

Áp vào task display, xem `app/app.h`:

```c
enum {
    /* [0] vùng hệ thống */
    AC_DISPLAY_RENDER_SCREEN = AK_SYS_DEFINE_SIG,

    /* [1..9] sự kiện nút, post từ app_bsp.cpp */
    AC_DISPLAY_BUTTON_MODE_PRESSED,
    AC_DISPLAY_BUTTON_MODE_LONG_PRESSED,
    AC_DISPLAY_BUTTON_MODE_RELEASED,
    AC_DISPLAY_BUTTON_UP_PRESSED,
    AC_DISPLAY_BUTTON_UP_LONG_PRESSED,
    AC_DISPLAY_BUTTON_UP_RELEASED,
    AC_DISPLAY_BUTTON_DOWN_PRESSED,
    AC_DISPLAY_BUTTON_DOWN_LONG_PRESSED,
    AC_DISPLAY_BUTTON_DOWN_RELEASED,

    /* [10..] signal cấp màn hình */
    AC_DISPLAY_INITIAL = AK_USER_DEFINE_SIG,
    AC_DISPLAY_SHOW_LOGO,
    AC_DISPLAY_SHOW_IDLE,
    AC_DISPLAY_SHOW_IDLE_BALL_MOVING_UPDATE,
    AC_DISPLAY_SHOW_FW_UPDATE,
    AC_DISPLAY_SHOW_FW_UPDATE_ERR,
};
```

Còn `SCREEN_ENTRY = 0xFE` với `SCREEN_EXIT = 0xFF` nằm ở `common/screen_manager.h`. Hai giá trị này để cao để khỏi đụng vùng nào.

## 2. Task display mỏng lắm

Toàn bộ `app/task_display.cpp`:

```c
scr_mng_t scr_mng_app;

void task_display(ak_msg_t* msg) {
    scr_mng_dispatch(msg);
}
```

Hết. Message nào tới `AC_TASK_DISPLAY_ID` cũng đẩy thẳng cho screen manager. Task này không có logic gì riêng.

## 3. Screen manager

`common/screen_manager.cpp`. Máy trạng thái mà trạng thái là một con trỏ hàm.

```c
typedef void (*screen_f)(ak_msg_t* msg);

typedef struct {
    screen_f screen;      // màn hình đang hiện
} scr_mng_t;
```

### Chuyển màn

```c
void scr_mng_tran(screen_f target, view_screen_t* scr_obj) {
    view_screen = scr_obj;
    screen_manager->screen = target;
    screen_manager->screen(&screen_msg_entry);   // bắn SCREEN_ENTRY cho màn mới
    view_render_screen(view_screen);
}
```

Dùng qua macro: `SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game)`.

Mỗi màn hình xử lý `SCREEN_ENTRY` để tự khởi tạo. Giống `onCreate()` bên Android.

### Giới hạn tần suất vẽ

Chỗ này là phần hay nhất về mặt kỹ thuật.

```c
static bool     screen_render_started = true;
static uint32_t screen_last_render_ms = 0;

static void scr_mng_render_screen() {
    uint32_t current_ms = sys_ctrl_millis();
    uint32_t time_diff  = current_ms - screen_last_render_ms;

    if (screen_render_started ||
        (time_diff >= AC_DISPLAY_MINIMUM_SCREEN_RENDER_INTERVAL_MS)) {
        screen_render_started = false;
        screen_last_render_ms = current_ms;
        view_render_screen(view_screen);              // vẽ ngay
    }
    else {
        timer_set(AC_TASK_DISPLAY_ID,                 // hẹn vẽ sau
                  AC_DISPLAY_RENDER_SCREEN,
                  AC_DISPLAY_MINIMUM_SCREEN_RENDER_INTERVAL_MS - time_diff,
                  TIMER_ONE_SHOT);
    }
}

void scr_mng_dispatch(ak_msg_t* msg) {
    screen_manager->screen(msg);    // để màn hình xử lý message
    scr_mng_render_screen();        // rồi mới tính chuyện vẽ
}
```

`AC_DISPLAY_MINIMUM_SCREEN_RENDER_INTERVAL_MS = 50`, tức tối đa 20 FPS.

Vì sao cần. Một lần vẽ là đẩy 1024 byte qua I2C bit-bang, chặn task suốt thời gian đó. Nếu cứ có message là vẽ, thì một loạt message dồn dập thành một loạt lần đẩy full-frame.

Cách gộp thì thế này. Chưa tới hạn thì hàm hẹn một timer one-shot. Mà `timer_set()` với cùng cặp `(task, sig)` sẽ thay thế timer đang chờ chứ không thêm cái mới. Nên nhiều message trong cùng khoảng 50 ms gộp lại thành đúng một lần vẽ.

Số đo thật, dùng harness dồn 20 message mỗi 200 ms trong 4 giây:

| | Số lần `update()` |
|---|---|
| Không giới hạn | 446 |
| Giới hạn 50 ms | 73 |

Khung hình cuối giống hệt nhau. Giảm 6 lần công việc I2C mà chẳng mất gì.

Còn `SCREEN_NONE_UPDATE_MASK()` để huỷ lần vẽ đã hẹn, cho màn hình nào biết khung vừa dựng đã cũ. Hiện chưa màn nào dùng.

## 4. View render

Có hai lớp.

`common/view_item.h` mô tả màn hình như dữ liệu:

```c
#define NUMBER_SCREEN_ITEMS_MAX  3

#define ITEM_TYPE_RECTANGLE  (0x00)   // ô chữ nhật có chữ, tự căn giữa
#define ITEM_TYPE_DYNAMIC    (0x01)   // gọi hàm vẽ tuỳ ý

typedef void (*dyn_render)();

typedef struct {
    view_item_t item;
    dyn_render  render;      // con trỏ hàm vẽ
} view_dynamic_t;

typedef struct {
    view_item_t* item[NUMBER_SCREEN_ITEMS_MAX];
    uint8_t focus_item;
} view_screen_t;
```

`common/view_render.cpp` duyệt các item rồi gọi hàm vẽ tương ứng:

```c
static view_render_item render_list[] = {
    view_render_rectangle,     // index = ITEM_TYPE_RECTANGLE
    view_render_dynamic        // index = ITEM_TYPE_DYNAMIC
};
```

Màn game dùng kiểu dynamic, tức một hàm vẽ duy nhất:

```c
view_dynamic_t dyn_view_item_dungeon_game = {
    { .item_type = ITEM_TYPE_DYNAMIC },
    view_scr_dungeon_game          // hàm vẽ toàn bộ
};

view_screen_t scr_dungeon_game = {
    &dyn_view_item_dungeon_game,
    ITEM_NULL,
    ITEM_NULL,
    .focus_item = 0,
};
```

Còn màn menu với setting dùng rectangle, tức khai báo bằng dữ liệu chứ không viết code vẽ.

## 5. Có những màn nào

Trong `app/screens/`:

| File | Màn hình |
|---|---|
| `scr_startup.cpp` | Logo AK Kernel, đọc setting, bật tắt tiếng |
| `scr_title.cpp` | Màn mở màn của game: tên game + hero + quái, chờ bấm MODE |
| `scr_menu_game.cpp` | Menu chính, có comment tiếng Việt hướng dẫn thêm item |
| `scr_dungeon_how_to_play.cpp` | Hướng dẫn chơi |
| `scr_dungeon_game.cpp` | Màn chơi chính, xem [05](05-game-dungeon.md) |
| `scr_game_over.cpp` | Hết ván, xếp hạng điểm |
| `scr_leaderboard.cpp` | Bảng thành tích |
| `scr_charts_game.cpp` | Biểu đồ điểm |
| `scr_game_setting.cpp` | Chỉnh party size, tốc độ quái, tốc độ animation, im lặng |
| `scr_idle.cpp` | Màn chờ khi không thao tác |
| `screens_bitmap.cpp` | Toàn bộ bitmap dạng mảng byte |

### Đi từ màn nào sang màn nào

Lấy trực tiếp từ mấy lời gọi `SCREEN_TRAN` trong code:

```raw
  startup            (logo AK Kernel, 2 giay hoac bam MODE de bo qua)
     │
     ▼
  title              (ten game, dung cho toi khi bam MODE)
     │
     ▼
  menu_game ◄──────────────────────────────────┐
     │                                          │
     ├──► game_setting ─────────────────────────┤
     ├──► leaderboard ──────────────────────────┤
     ├──► idle ─────────────────────────────────┤
     ├──► charts_game ──┬───────────────────────┤
     │                  └──► dungeon_how_to_play│
     │                            │             │
     └──► dungeon_how_to_play ────┤             │
                                  ├─────────────┘
                                  ▼
                            dungeon_game
                                  │
                                  ▼
                            game_over
                                  │
                    ┌─────────────┼──────────────┐
                    ▼             ▼              ▼
              menu_game     charts_game   dungeon_how_to_play
```

Điểm đáng chú ý: gần như màn nào cũng có đường về `menu_game`. Chỉ `dungeon_game` là không, nó bắt buộc đi qua `game_over`.

Hai màn đầu chia việc rõ ràng: `startup` là của kernel, nó khoe "con này chạy AK";
`title` là của game, nó khoe "con này là Dungeon". `startup` tự nhảy sau 2 giây,
còn `title` thì đứng chờ, phải bấm MODE mới đi tiếp.

`title` có một timer `TIMER_PERIODIC` 500 ms cho dòng "PRESS MODE" nhấp nháy.
Screen manager của AK **không** phát `SCREEN_EXIT`, nên không có chỗ nào tự dọn
timer hộ. Vì vậy nhánh xử lý nút MODE phải gọi `timer_remove_attr()` trước khi
`SCREEN_TRAN`. Quên là timer cứ nổ suốt đời máy chạy, mỗi lần nổ lại kéo theo
một lần đẩy nguyên khung hình 1024 byte qua I2C. Màn `scr_idle` cũng cùng kiểu
bẫy đó, xem cách nó dọn `AC_DISPLAY_SHOW_IDLE_BALL_MOVING_UPDATE`.

Muốn xem lại sơ đồ này sau khi thêm màn mới thì chạy:

```bash
cd application/sources/app/screens
for f in *.cpp; do
  t=$(grep -oE "SCREEN_TRAN\(scr_[a-z_]+_handle" $f | sed 's/SCREEN_TRAN(scr_//;s/_handle//' | sort -u | tr '\n' ' ')
  [ -n "$t" ] && printf "%-28s -> %s\n" "${f%.cpp}" "$t"
done
```

Màn hình nào cũng có khung như nhau:

```c
static void view_scr_xxx() {
    view_render.clear();
    /* ...vẽ vào framebuffer... */
}

void scr_xxx_handle(ak_msg_t* msg) {
    switch (msg->sig) {
    case SCREEN_ENTRY:
        view_render.initialize();
        view_render_display_on();
        /* đọc dữ liệu, hẹn timer */
        break;

    case AC_DISPLAY_BUTTON_MODE_RELEASED:
        SCREEN_TRAN(scr_khac_handle, &scr_khac);
        BUZZER_PlayTones(tones_cc);
        break;
    ...
    }
}
```

## 6. Mấy task hệ thống

### task_life, ưu tiên 6

```c
case AC_LIFE_SYSTEM_CHECK:
    sys_ctrl_independent_watchdog_reset();
    sys_ctrl_soft_watchdog_reset();
    led_toggle(&led_life);
```

Hẹn giờ lặp 1000 ms từ `app_start_timer()`.

Ưu tiên 6, cao thứ nhì sau timer. Cố ý đấy. Vỗ watchdog mà xếp sau game thì hỏng.

### task_shell với shell.cpp, ưu tiên 2

Nhận dòng lệnh từ `task_polling_console()`, phân giải bằng `common/cmd_line.c`.

Lệnh có sẵn, xem `app/shell.cpp`:

| Lệnh | Việc |
|---|---|
| `help` | Liệt kê lệnh |
| `reboot` / `reset` | Khởi động lại |
| `fatal` | In fatal log từ W25Q80: mã lỗi, số lần restart |
| `ram` | Mức dùng RAM và stack |
| `flash` | Thao tác W25Q80 |
| `epprom` / `eps` | Dump hoặc ghi EEPROM nội |
| `lcd` | Test màn hình |
| `beep` | Test buzzer |
| `boot` | Thông tin bootloader, kích hoạt update |

Bo đóng hộp rồi thì đây là công cụ debug mạnh nhất. Chỉ cần cáp UART.

### task_fw, ưu tiên 2

Máy trạng thái cập nhật firmware. Signal ở `app/task_fw.cpp`:

```raw
FW_CHECKING_REQ                 tự chạy sau 5 s, do app_start_timer hẹn
FW_CRENT_APP_FW_INFO_REQ        host hỏi phiên bản app
FW_CRENT_BOOT_FW_INFO_REQ       host hỏi phiên bản bootloader
FW_UPDATE_REQ                   host xin cập nhật
FW_UPDATE_SM_OK / SM_BUSY       đồng ý / đang bận
FW_TRANSFER_REQ                 nhận từng gói dữ liệu
FW_PACKED_TIMEOUT               quá 5 s không có gói thì huỷ
FW_INTERNAL_UPDATE_APP_RES_OK   ghi xong app
FW_INTERNAL_UPDATE_BOOT_RES_OK  ghi xong bootloader
FW_SAFE_MODE_RES_OK             vào chế độ an toàn
```

### task_if với task_uart_if, ưu tiên 4

Cặp task định tuyến message ra vào bên ngoài:

```raw
task_if       : AC_IF_{PURE,COMMON,DYNAMIC}_MSG_{IN,OUT}
task_uart_if  : AC_UART_IF_{PURE,COMMON,DYNAMIC}_MSG_{IN,OUT}
```

`task_if` là lớp trừu tượng "giao tiếp với bên ngoài" nói chung. `task_uart_if` là hiện thực cụ thể qua UART.

Tách ra để sau này muốn thêm BLE hay RF thì chỉ cần viết thêm một `task_xxx_if`, khỏi đụng `task_if`.

Xem [07-networks-link.md](07-networks-link.md).

## 7. main_app khởi tạo cái gì trước

`app/app.cpp:93`. Thứ tự có lý do cả:

```c
/* 1. Kernel trước. Từ đây trở đi mọi thứ đều post message được */
ENTRY_CRITICAL();
task_init();
task_create((task_t*)app_task_table);
task_polling_create((task_polling_t*)app_task_polling_table);
EXIT_CRITICAL();

/* 2. Watchdog sớm. Từ giờ treo là bị reset */
sys_ctrl_independent_watchdog_init();   /* 32s */
sys_ctrl_soft_watchdog_init(200);       /* 20s */

/* 3. Ngoại vi */
SPI.begin();
io_cfg_adc1();
adc_bat_io_cfg();
flash_io_ctrl_init();

/* 4. Đối tượng phần mềm */
sys_boot_init();
led_init(&led_life, led_life_init, led_life_on, led_life_off);
ring_buffer_char_init(&ring_buffer_console_rev, buffer_console_rev, BUFFER_CONSOLE_REV_SIZE);
button_init(&btn_mode, 10, BUTTON_MODE_ID, io_button_mode_init, io_button_mode_read, btn_mode_callback);
button_init(&btn_up,   10, BUTTON_UP_ID,   io_button_up_init,   io_button_up_read,   btn_up_callback);
button_init(&btn_down, 10, BUTTON_DOWN_ID, io_button_down_init, io_button_down_read, btn_down_callback);
button_enable(&btn_mode); button_enable(&btn_up); button_enable(&btn_down);
BUZZER_Init();

/* 5. Đọc dữ liệu khởi động */
flash_read(APP_FLASH_INTTERNAL_SHARE_DATA_SECTOR_1, ..., sizeof(boot_app_share_data_t));
if (boot_app_share_data.is_power_on_reset == SYS_POWER_ON_RESET) app_power_on_reset();
/* tăng bộ đếm restart trong fatal log */

/* 6. Hẹn giờ với màn hình đầu tiên */
app_start_timer();
app_task_init();     // SCREEN_CTOR(&scr_mng_app, scr_startup_handle, &scr_startup)

/* 7. Giao quyền cho kernel, không bao giờ quay lại */
return task_run();
```

`app_start_timer()` hẹn 3 cái:

```c
timer_set(AC_TASK_LIFE_ID,    AC_LIFE_SYSTEM_CHECK, 1000, TIMER_PERIODIC);   // LED + watchdog
timer_set(AC_TASK_FW_ID,      FW_CHECKING_REQ,      5000, TIMER_ONE_SHOT);   // hỏi update 1 lần
timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_INITIAL,    100, TIMER_ONE_SHOT);   // khởi động màn hình
```
