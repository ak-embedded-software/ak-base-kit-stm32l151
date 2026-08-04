# 10 — Hướng dẫn mở rộng

Mấy công thức từng bước. Làm theo là chạy.

Sau mỗi mục đều có phần "kiểm lại" để biết đã đủ chưa.

---

## 1. Thêm một task mới

Ví dụ thêm task `dungeon_sound` lo âm thanh, để mấy task khác khỏi gọi buzzer trực tiếp.

### Bước 1 — Khai ID trong `app/task_list.h`

```c
enum {
    TASK_TIMER_TICK_ID,

    AC_TASK_FW_ID,
    AC_TASK_SHELL_ID,
    AC_TASK_LIFE_ID,
    AC_TASK_IF_ID,
    AC_TASK_UART_IF_ID,
    AC_TASK_DISPLAY_ID,

    DUNGEON_STATE_ID,
    DUNGEON_LANE_ID,
    DUNGEON_CONTROL_ID,
    DUNGEON_ACTION_ID,
    DUNGEON_EFFECT_ID,
    DUNGEON_SOUND_ID,        /* <-- THÊM Ở ĐÂY */
    DUNGEON_SCREEN_ID,
    ...
    AK_TASK_EOT_ID,
};
```

ID **phải tăng dần liên tục**, không được nhảy số. Kernel dùng nó làm index vào mảng `task_table`.

Chèn vào giữa cũng được, mấy ID sau tự dịch lên. Không sao cả, vì không chỗ nào lưu ID ra ngoài RAM.

### Bước 2 — Khai hàm xử lý, cũng trong `task_list.h`

```c
extern void dungeon_sound_handle(ak_msg_t*);
```

### Bước 3 — Thêm dòng vào bảng, trong `app/task_list.cpp`

```c
{DUNGEON_EFFECT_ID , TASK_PRI_LEVEL_4, dungeon_effect_handle},
{DUNGEON_SOUND_ID  , TASK_PRI_LEVEL_4, dungeon_sound_handle },   /* <-- THÊM */
{DUNGEON_SCREEN_ID , TASK_PRI_LEVEL_4, scr_dungeon_game_handle},
```

Thứ tự dòng trong bảng **phải khớp thứ tự enum**. Lệch là task này nhận message của task kia.

Chọn mức ưu tiên thế nào:

| Mức | Dùng cho | Ví dụ đang có |
|---|---|---|
| 7 | Nhịp hệ thống | `task_timer_tick` |
| 6 | Sống còn | `task_life` (watchdog) |
| 5 | Tầng giao thức trên | `task_link` |
| 4 | Việc bình thường | game, display, if |
| 3 | Tầng giao thức dưới | `task_link_phy` |
| 2 | Việc chậm, không gấp | `task_fw`, `task_shell` |
| 1 | Chưa dùng | |
| 0 | **Chỉ cho dòng EOT** | |

Không được đặt `TASK_PRI_LEVEL_0` cho task thật. `task_post` tính `task_pri_queue[pri - 1]`, mức 0 là index `-1`, hỏng bộ nhớ ngay.

### Bước 4 — Khai signal, trong `app/app.h`

```c
/*****************************************************************************/
/*  Dungeon game sound task define
 */
/*****************************************************************************/
enum {
    DUNGEON_SOUND_PLAY_HIT = DUNGEON_DEFINE_SIG,
    DUNGEON_SOUND_PLAY_WIN,
    DUNGEON_SOUND_PLAY_LOSE,
    DUNGEON_SOUND_MUTE,
};
```

Bắt đầu bằng `DUNGEON_DEFINE_SIG` (=100) cho signal game. Nếu là task hệ thống thì dùng `AK_USER_DEFINE_SIG` (=10).

Nhớ là mỗi task có không gian signal riêng. Task này dùng số 100 không đụng gì task khác cũng dùng 100.

### Bước 5 — Viết file `.h`

`app/game/dungeon_game/dungeon_sound.h`:

```c
#ifndef __DUNGEON_SOUND_H__
#define __DUNGEON_SOUND_H__

#include <stdio.h>

#include "fsm.h"
#include "port.h"
#include "message.h"
#include "timer.h"

#include "app.h"
#include "app_dbg.h"
#include "task_list.h"
#include "task_display.h"

#include "buzzer.h"
#include "scr_dungeon_game.h"

extern void dungeon_sound_handle(ak_msg_t* msg);

#endif //__DUNGEON_SOUND_H__
```

Copy y khối include của `dungeon_effect.h` là nhanh nhất.

### Bước 6 — Viết file `.cpp`

`app/game/dungeon_game/dungeon_sound.cpp`:

```c
#include "dungeon_sound.h"
#include "dungeon_runtime.h"

static uint8_t sound_muted = 0;

void dungeon_sound_handle(ak_msg_t* msg) {
    switch (msg->sig) {
    case DUNGEON_SOUND_PLAY_HIT: {
        APP_DBG_SIG("DUNGEON_SOUND_PLAY_HIT\n");
        if (!sound_muted) BUZZER_PlayTones(tones_cc);
    }
        break;

    case DUNGEON_SOUND_PLAY_WIN: {
        APP_DBG_SIG("DUNGEON_SOUND_PLAY_WIN\n");
        if (!sound_muted) BUZZER_PlayTones(tones_startup);
    }
        break;

    case DUNGEON_SOUND_MUTE: {
        APP_DBG_SIG("DUNGEON_SOUND_MUTE\n");
        sound_muted = 1;
    }
        break;

    default:
        break;
    }
}
```

Luật bất di bất dịch: **làm xong thì return**. Không `while`, không `delay`.

### Bước 7 — Thêm vào build

`app/game/dungeon_game/Makefile.mk`:

```makefile
SOURCES_CPP += sources/app/game/dungeon_game/dungeon_sound.cpp
```

Quên bước này thì lỗi `undefined reference to dungeon_sound_handle`.

### Bước 8 — Gửi message cho nó

Từ bất kỳ task nào:

```c
task_post_pure_msg(DUNGEON_SOUND_ID, DUNGEON_SOUND_PLAY_HIT);
```

### Kiểm lại

```raw
[ ] enum trong task_list.h tăng dần liên tục
[ ] extern hàm handle trong task_list.h
[ ] dòng trong app_task_table đúng thứ tự với enum
[ ] mức ưu tiên >= 1
[ ] signal khai trong app.h
[ ] file .cpp thêm vào Makefile.mk
[ ] make clean && make chạy được
[ ] bật -DAPP_DBG_SIG_EN, xem UART có in signal không
```

---

## 2. Thêm một màn hình

### Trước khi vẽ: luật bố cục chung

Mọi màn trong game đều theo một luật duy nhất, ghi ở `app/screens/screens_layout.h`.
Đọc cái này trước rồi hãy vẽ, không thì màn mới sẽ lệch tông với mấy màn cũ.

Màn OLED là 128x64. Không màn nào vẽ khung viền ngoài nữa. Thay vào đó mọi thứ
chừa lề 3 px bốn phía:

| Hằng số | Giá trị | Nghĩa |
|---|---|---|
| `SCR_PAD_L` / `SCR_PAD_R` | 3 / 124 | cột trái nhất và phải nhất được vẽ |
| `SCR_PAD_T` / `SCR_PAD_B` | 3 / 60 | dòng trên nhất và dưới nhất được vẽ |
| `SCR_USABLE_W` | 122 | bề ngang vẽ được, đúng 20 ký tự |
| `SCR_CHAR_W` / `SCR_CHAR_H` | 6 / 8 | một ký tự cỡ 1 chiếm bao nhiêu px |
| `SCR_ROW_TITLE` | 3 | hàng tiêu đề |
| `SCR_ROW_RULE` | 11 | đường kẻ dưới tiêu đề |
| `SCR_ROW_BODY` | 15 | hàng nội dung đầu tiên |
| `SCR_ROW_HINT` | 53 | hàng gợi ý thao tác ở đáy |
| `SCR_CENTER_X(n)` | hàm | cột x để canh giữa chuỗi n ký tự |
| `SCR_CENTER_X_BIG(n)` | hàm | như trên nhưng cho chữ cỡ 2 |
| `SCR_MAX_CHARS(x)` | hàm | chuỗi bắt đầu ở cột x nhét được mấy ký tự |

**Cái bẫy hay dính nhất:** font `glcdfont` là 5x8 chứ không phải 5x7. Chữ cỡ 1
đặt ở `y` sẽ chiếm `y` tới `y+7`. Phần lớn ký tự chỉ dùng 7 dòng trên nên nhìn
qua tưởng cao 7. Nhưng mấy chữ có nét thả xuống như `y` `g` `p` `q` dùng tới
dòng thứ 8. Nên hàng chữ cuối cùng phải đặt ở `y <= 53`, không phải 54.

Vùng dọc chỉ có 58 dòng nên mấy cách chia hàng hay dùng là:

- 3 thẻ cao 18, khe 2 px → `18*3 + 2*2 = 58` (màn Menu dùng cái này)
- 4 hàng cao 13, khe 2 px → `13*4 + 2*3 = 58` (màn Setting dùng cái này)

Chỉ có hai chỗ cố ý tràn viền: dải hang ở màn TRAVEL (nó là đường đi, phải chạm
hai rìa mới ra cảm giác hang dài) và màn `scr_idle` (screensaver bóng nảy, nảy
chạm mép mới đúng). Ngoài hai chỗ đó, mọi pixel phải nằm trong lề.

### Rồi mới tới màn mới

Ví dụ thêm màn "Credits".

### Bước 1 — File `.h`

`app/screens/scr_credits.h`:

```c
#ifndef __SCR_CREDITS_H__
#define __SCR_CREDITS_H__

#include "fsm.h"
#include "port.h"
#include "message.h"
#include "timer.h"

#include "app.h"
#include "app_dbg.h"
#include "task_list.h"
#include "task_display.h"
#include "view_render.h"

#include "screens.h"
#include "screens_bitmap.h"

#endif //__SCR_CREDITS_H__
```

### Bước 2 — File `.cpp`

`app/screens/scr_credits.cpp`:

```c
#include "scr_credits.h"

static void view_scr_credits();

view_dynamic_t dyn_view_item_credits = {
    { .item_type = ITEM_TYPE_DYNAMIC },
    view_scr_credits
};

view_screen_t scr_credits = {
    &dyn_view_item_credits,
    ITEM_NULL,
    ITEM_NULL,
    .focus_item = 0,
};

static void view_scr_credits() {
    view_render.clear();
    view_render.setTextSize(1);
    view_render.setTextColor(WHITE);

    /* Dùng hằng số trong screens_layout.h, đừng gõ toạ độ tay.
     * SCR_CENTER_X(n) trả về cột x để canh giữa chuỗi n ký tự. */
    view_render.setCursor(SCR_CENTER_X(12), SCR_ROW_TITLE);
    view_render.print("DUNGEON GAME");
    view_render.drawLine(SCR_PAD_L, SCR_ROW_RULE, SCR_PAD_R, SCR_ROW_RULE, WHITE);

    view_render.setCursor(SCR_PAD_L, SCR_ROW_BODY);
    view_render.print("by An Nguyen");

    view_render.setCursor(SCR_CENTER_X(11), SCR_ROW_HINT);
    view_render.print("MODE = Back");
}

void scr_credits_handle(ak_msg_t* msg) {
    switch (msg->sig) {
    case SCREEN_ENTRY: {
        APP_DBG_SIG("SCREEN_ENTRY\n");
        view_render.initialize();
        view_render_display_on();
    }
        break;

    case AC_DISPLAY_BUTTON_MODE_RELEASED: {
        APP_DBG_SIG("AC_DISPLAY_BUTTON_MODE_RELEASED\n");
        SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
        BUZZER_PlayTones(tones_cc);
    }
        break;

    default:
        break;
    }
}
```

### Bước 3 — Khai trong `app/screens/screens.h`

Thêm include ở đầu:

```c
#include "scr_credits.h"
```

Rồi khai 3 thứ ở dưới:

```c
// scr_credits
extern view_dynamic_t dyn_view_item_credits;
extern view_screen_t scr_credits;
extern void scr_credits_handle(ak_msg_t* msg);
```

### Bước 4 — Thêm vào build

`app/screens/Makefile.mk`:

```makefile
SOURCES_CPP += sources/app/screens/scr_credits.cpp
```

### Bước 5 — Có đường vào

Từ màn nào đó gọi:

```c
SCREEN_TRAN(scr_credits_handle, &scr_credits);
```

Muốn vào từ menu thì xem mục 3 ngay dưới.

### Kiểm lại

```raw
[ ] view_dynamic_t và view_screen_t đều khai
[ ] 3 dòng extern trong screens.h
[ ] include scr_credits.h trong screens.h
[ ] thêm vào Makefile.mk
[ ] có ít nhất 1 nút để thoát ra, không thì kẹt luôn
```

Bẫy hay gặp nhất: quên xử lý nút thoát. Vào rồi ra không được, phải reset bo.

---

## 3. Thêm item vào menu chính

`scr_menu_game.cpp` có sẵn hướng dẫn tiếng Việt ở đầu file. Đây là bản chi tiết hơn.

### Bước 1 — Chuẩn bị icon

Ảnh đen trắng, cỡ 13 tới 18 pixel mỗi chiều. Đổi sang mảng byte bằng công cụ như image2cpp, rồi bỏ vào `screens_bitmap.cpp`.

### Bước 2 — Tăng số lượng

```c
#define NUMBER_MENU_ITEMS  (7)     // đang là 6
```

### Bước 3 — Thêm định danh

```c
enum {
    MENU_ITEM_CONTINUE = 0,
    MENU_ITEM_NEW_GAME,
    MENU_ITEM_LEVEL_INFO,
    MENU_ITEM_LEADERBOARD,
    ...
    MENU_ITEM_CREDITS,      /* <-- THÊM */
};
```

### Bước 4 — Thêm tên, icon, kích thước

Bốn mảng này phải cùng số phần tử, cùng thứ tự với enum:

```c
menu_items_name[]          // tên hiện trên màn
menu_items_icon[]          // con trỏ tới bitmap
menu_items_icon_size_w[]   // chiều rộng
menu_items_icon_size_h[]   // chiều cao
```

Thêm thiếu một mảng là đọc ngoài mảng. Không crash ngay đâu, nó chỉ vẽ rác.

### Bước 5 — Nối vào màn đích

Tìm hàm `screen_tran_menu()` rồi thêm nhánh:

```c
case MENU_ITEM_CREDITS:
    SCREEN_TRAN(scr_credits_handle, &scr_credits);
    break;
```

Icon lệch thì chỉnh `menu_items_icon_axis_y[]`.

---

## 4. Thêm một con quái

Ví dụ thêm `SKELETON`.

### Bước 1 — Enum, trong `dungeon_runtime.h`

```c
enum {
    DUNGEON_MONSTER_SLIME = 0,
    DUNGEON_MONSTER_GOBLIN,
    DUNGEON_MONSTER_WOLF,
    DUNGEON_MONSTER_GORILLA,
    DUNGEON_MONSTER_DRAGON,
    DUNGEON_MONSTER_EYE,
    DUNGEON_MONSTER_SKELETON,    /* <-- THÊM Ở CUỐI */
};
```

**Thêm ở cuối**, đừng chèn vào giữa. Vì `current_monster` được lưu vào EEPROM ở `dungeon_game_save_t`. Chèn giữa là save cũ đọc lên thành con quái khác.

### Bước 2 — Tên, trong `dungeon_runtime.cpp`

```c
const char* dungeon_monster_name[DUNGEON_MONSTER_EYE + 1] = {
```

Sửa kích thước mảng luôn, không thì tràn:

```c
const char* dungeon_monster_name[DUNGEON_MONSTER_SKELETON + 1] = {
    "SLIME", "GOBLIN", "WOLF", "GORILLA", "DRAGON", "EYE WATCHER",
    "SKELETON",
};
```

Tên dài quá thì tràn khỏi màn. Cỡ chữ 1 vẽ mỗi ký tự 6 px, mà vùng vẽ có lề 3 px
hai bên nên chỉ còn 122 px, tức tối đa **20 ký tự**. Muốn chắc thì dùng
`SCR_MAX_CHARS(x)` trong `screens_layout.h` để biết chuỗi bắt đầu ở cột `x`
nhét được bao nhiêu ký tự.

### Bước 3 — Chỉ số, trong `dungeon_state.cpp`

Vào `dungeon_set_monster_stats()`, thêm case:

```c
case DUNGEON_MONSTER_SKELETON:
    dungeon_runtime.monster_max_hp = 110;
    dungeon_runtime.monster_dmg    = 35;
    dungeon_runtime.monster_armor  = 3 + dungeon_runtime.level;
    break;
```

Để ý cái `default:` ở cuối switch đang gán chỉ số của EYE WATCHER. Không thêm case riêng thì quái mới sẽ mang chỉ số đó.

### Bước 4 — Chiêu riêng, cũng trong `dungeon_state.cpp`

Vào `dungeon_enemy_action()`, thêm nhánh:

```c
else if ((dungeon_runtime.current_monster == DUNGEON_MONSTER_SKELETON) &&
         dungeon_turn_matches(dungeon_runtime.battle_turn, 2, 4)) {
    dungeon_runtime.monster_armor += 3;      /* lượt 2, 6, 10... cộng giáp */
}
```

`dungeon_turn_matches(turn, first, step)` nghĩa là: lượt `first`, rồi cứ mỗi `step` lượt một lần.

### Bước 5 — Cho nó xuất hiện

Vào `dungeon_monster_table[5][8]` trong `dungeon_runtime.cpp`. Hàng là level 1..5, cột là stage 1..8:

```c
const uint8_t dungeon_monster_table[5][8] = {
    {SLIME, GOBLIN, WOLF, GORILLA, ...},                  // level 1
    ...
    {SLIME, GOBLIN, GOBLIN, WOLF, SKELETON, GORILLA, DRAGON, EYE},   // level 5
};
```

Nhớ là level N chỉ dùng `dungeon_stage_counts[N-1]` cột đầu tiên. Level 1 có 4 stage nên chỉ đọc 4 cột đầu.

### Bước 6 — Bitmap

Vào `dungeon_monster_bitmap()` trong `scr_dungeon_game.cpp`:

```c
case DUNGEON_MONSTER_SKELETON:
    return {monster_skeleton, 30, 30};
```

Bitmap khai trong `screens_bitmap.cpp`, extern trong `screens_bitmap.h`.

### Kiểm lại

```raw
[ ] enum thêm ở CUỐI
[ ] kích thước mảng dungeon_monster_name sửa theo
[ ] case trong dungeon_set_monster_stats
[ ] có mặt trong dungeon_monster_table
[ ] case trong dungeon_monster_bitmap
[ ] tên không quá 21 ký tự
[ ] chơi thử tới stage có nó
```

---

## 5. Thêm một item

### Bước 1 — Enum, trong `dungeon_runtime.h`

```c
enum {
    DUNGEON_ITEM_SWORD = 0,
    ...
    DUNGEON_ITEM_POISON,
    DUNGEON_ITEM_ELIXIR,     /* <-- THÊM trước COUNT */
    DUNGEON_ITEM_COUNT,
};
```

Phải đặt **trước** `DUNGEON_ITEM_COUNT`. Vì `COUNT` được dùng làm kích thước mảng:

```c
uint8_t inventory[DUNGEON_ITEM_COUNT];
```

### Bước 2 — Coi chừng chỗ save

`dungeon_game_save_t` trong `app_eeprom.h` có:

```c
uint8_t inventory[7];      /* <-- số cứng, không phải DUNGEON_ITEM_COUNT */
```

Thêm item thứ 8 mà không sửa số này là `memcpy` trong `dungeon_save_progress()` chỉ chép 7 phần tử. Item mới không được lưu.

Sửa thành 8, và biết rằng save cũ sẽ không đọc được nữa (kích thước struct đổi, checksum sai, rơi về mặc định).

### Bước 3 — Tên, trong `dungeon_runtime.cpp`

```c
const char* dungeon_item_name[DUNGEON_ITEM_COUNT] = {
    "Sword", "Shield", "Healing", "Bomb", "Antidote", "Purify", "Poison",
    "Elixir",
};
```

### Bước 4 — Tác dụng khi nhặt, trong `dungeon_control.cpp`

Vào `dungeon_apply_chest_item()`:

```c
switch (item) {
case DUNGEON_ITEM_SWORD:
    dungeon_runtime.player_atk += 5 + (level_bonus * 3);
    break;
case DUNGEON_ITEM_SHIELD:
    dungeon_runtime.player_def += 5 + (level_bonus * 4);
    break;
case DUNGEON_ITEM_ELIXIR:                        /* <-- THÊM */
    dungeon_runtime.player_max_hp += 10;
    dungeon_runtime.player_hp = dungeon_runtime.player_max_hp;
    break;
default:
    dungeon_runtime.inventory[item]++;           /* mặc định là cất vào túi */
    break;
}
```

Nhánh `default` cất item vào túi. Muốn dùng ngay thì viết case riêng như trên.

### Bước 5 — Tác dụng khi dùng trong trận

Nếu item cất vào túi thì vào `dungeon_use_best_item()` ở `dungeon_action.cpp`, thêm luật chọn và tác dụng.

### Bước 6 — Bitmap

Vào `dungeon_item_bitmap()` trong `scr_dungeon_game.cpp`:

```c
case DUNGEON_ITEM_ELIXIR:
    return {item_elixir, 20, 20};
```

### Kiểm lại

```raw
[ ] enum đặt TRƯỚC DUNGEON_ITEM_COUNT
[ ] sửa inventory[7] trong app_eeprom.h nếu vượt 7
[ ] tên trong dungeon_item_name
[ ] case trong dungeon_apply_chest_item
[ ] case trong dungeon_item_bitmap
[ ] biết là save cũ sẽ mất nếu đổi kích thước struct
```

---

## 6. Thêm một lệnh shell

### Bước 1 — Khai hàm, trong `app/shell.cpp`

Chỗ khai với mấy hàm khác, quanh dòng 66:

```c
int32_t shell_dungeon(uint8_t* argv);
```

Chữ ký cố định: nhận `uint8_t*`, trả `int32_t`.

### Bước 2 — Thêm vào bảng lệnh

Trong `lgn_cmd_table[]`, khoảng dòng 78:

```c
const cmd_line_t lgn_cmd_table[] = {
    {(const int8_t*)"reset",   shell_reset,   (const int8_t*)"reset terminal"},
    {(const int8_t*)"help",    shell_help,    (const int8_t*)"help info"},
    ...
    {(const int8_t*)"dungeon", shell_dungeon, (const int8_t*)"dungeon runtime info"},  /* <-- THÊM */

    /* End Of Table */
    {(const int8_t*)0,(pf_cmd_func)0,(const int8_t*)0}
};
```

Phải thêm **trước** dòng End Of Table. Bảng kết thúc bằng con trỏ NULL, thêm sau là không ai thấy.

Ba cột: chuỗi lệnh, hàm, mô tả hiện khi gõ `help`.

### Bước 3 — Viết hàm

```c
int32_t shell_dungeon(uint8_t* argv) {
    (void)argv;
    LOGIN_PRINT("level      : %d\n", dungeon_runtime.level);
    LOGIN_PRINT("stage      : %d/%d\n", dungeon_runtime.stage,
                                        dungeon_runtime.total_stages);
    LOGIN_PRINT("player hp  : %d/%d\n", dungeon_runtime.player_hp,
                                        dungeon_runtime.player_max_hp);
    LOGIN_PRINT("monster hp : %d/%d\n", dungeon_runtime.monster_hp,
                                        dungeon_runtime.monster_max_hp);
    LOGIN_PRINT("view       : %d\n", dungeon_runtime.current_view);
    LOGIN_PRINT("phase      : %d\n", dungeon_runtime.battle_phase);
    LOGIN_PRINT("score      : %lu\n", (unsigned long)dungeon_game_score);
    return 0;
}
```

Nhớ `#include "dungeon_runtime.h"` ở đầu `shell.cpp`.

Dùng `LOGIN_PRINT` chứ đừng `printf`. Macro này ở `app/app_dbg.h`, nó đi qua `xprintf` để ra đúng cổng UART.

### Bước 4 — Thử

```bash
make && make flash
make com dev=/dev/ttyUSB0
```

Rồi gõ `dungeon`.

### Kiểm lại

```raw
[ ] khai hàm trước bảng
[ ] dòng trong bảng đặt TRƯỚC End Of Table
[ ] dùng LOGIN_PRINT
[ ] gõ help thấy lệnh mới
```

---

## 7. Thêm timer định kỳ

Muốn task nào đó tự chạy mỗi N mili giây:

```c
timer_set(DUNGEON_SOUND_ID,          /* task nhận */
          DUNGEON_SOUND_PLAY_HIT,    /* signal sẽ bắn */
          500,                       /* mili giây */
          TIMER_PERIODIC);           /* hoặc TIMER_ONE_SHOT */
```

Huỷ:

```c
timer_remove_attr(DUNGEON_SOUND_ID, DUNGEON_SOUND_PLAY_HIT);
```

Ba điều cần nhớ.

Pool chỉ có 16 timer. Hết là `FATAL`.

Gọi `timer_set` với cùng cặp `(task, sig)` thì nó **thay thế** timer cũ, không tạo thêm. Đặc tính này chính là thứ mà `screen_manager` lợi dụng để gộp lần vẽ.

Chuyển màn hình mà quên `timer_remove_attr` thì timer cũ vẫn bắn signal cho màn mới. Màn mới không hiểu signal đó thì rơi vào `default:` rồi bị bỏ qua, nhưng vẫn tốn một lần dispatch và một lần vẽ.

---

## 8. Đổi tốc độ game

| Muốn đổi | Sửa ở đâu | Hiện tại |
|---|---|---|
| Nhịp game | `DUNGEON_TIME_TICK_INTERVAL` trong `app.h` | 100 ms |
| Tốc độ animation đánh | `DUNGEON_BATTLE_STEP_TICKS` trong `dungeon_runtime.h` | 5 tick |
| Thời gian chờ giữa lượt | `DUNGEON_BATTLE_WAIT_TICKS` | 10 tick |
| Thời gian rung hình | `DUNGEON_SHAKE_TICKS` | 6 tick |
| Thời gian hiện số damage | `DUNGEON_POPUP_TICKS` | 24 tick |
| FPS tối đa | `AC_DISPLAY_MINIMUM_SCREEN_RENDER_INTERVAL_MS` trong `app.h` | 50 ms (20 FPS) |

Mấy con số "tick" đếm theo nhịp game, không phải mili giây. `DUNGEON_BATTLE_STEP_TICKS = 5` với tick 100 ms là 500 ms thật.

Giảm `DUNGEON_TIME_TICK_INTERVAL` xuống 50 ms thì game nhanh gấp đôi, nhưng CPU cũng phải xử lý gấp đôi số message. Kiểm lại `waitTime` bằng cách bật `AK_TASK_LOG_CONSOLE_ENABLE`.

---

## 9. Mấy cái đừng làm

**Đừng `delay()` hay `while` chờ trong task.** Cả hệ thống đứng. Cần chờ thì hẹn timer rồi return.

**Đừng gọi hàm nặng trong callback nút.** Callback chạy trong ngắt. Chỉ post message rồi thoát.

**Đừng chèn enum vào giữa nếu giá trị đó được lưu EEPROM.** Thêm ở cuối. Áp dụng cho `DUNGEON_MONSTER_*`, `DUNGEON_ITEM_*`, `battle_phase`.

**Đừng đặt mảng lớn trên stack.** Chỉ còn khoảng 2.3 KB. Cần buffer to thì khai `static`.

**Đừng quên `make clean` sau khi xoá hay đổi tên file.**

**Đừng nạp vào `0x08000000`.** Đó là bootloader. Application ở `0x08003000`.
