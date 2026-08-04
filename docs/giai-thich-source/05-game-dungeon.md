# 05 — Game Dungeon

Phần này bị sửa nhiều nhất.

Trước đó cả 1616 dòng luật chơi nằm hết trong `scr_dungeon_game.cpp`, còn 5 task game thì gần như rỗng. Giờ luật chơi đã về đúng chủ, màn hình chỉ còn lo hiển thị.

## 1. Ai lo phần nào

```raw
                      ┌────────────────────────┐
                      │   dungeon_runtime      │  state chung của 1 ván
                      │   (dungeon_runtime.h)  │
                      └───────────┬────────────┘
                                  │ mọi task đọc ghi
        ┌──────────┬──────────┬───┴──────┬──────────┬──────────┐
        ▼          ▼          ▼          ▼          ▼          ▼
  dungeon_    dungeon_   dungeon_   dungeon_   dungeon_   scr_dungeon
   state       lane      control     action     effect       _game
  ────────   ────────   ────────   ────────   ────────   ───────────
  quái:      cấp/màn:   chọn lệnh: giải lượt: hiệu ứng:  CHỈ VẼ
  stats      tiến độ    di chuyển  máy pha    popup      draw_travel
  AI đánh    travel     con trỏ    đánh       số damage  draw_battle
  status     lưu đọc    mở rương   dungeon_   rung hình  draw_chest
  effect     điểm       dùng item  tick()                draw_message
```

| File | Dòng | Nội dung chính |
|---|---|---|
| `dungeon_runtime.h/.cpp` | 80 | struct state chung, enum, bảng dữ liệu, helper nhỏ |
| `dungeon_state.cpp` | 216 | `dungeon_set_monster_stats`, `dungeon_enemy_action`, `dungeon_status_tick`, `dungeon_monster_turn` |
| `dungeon_lane.cpp` | 357 | `dungeon_init_player`, `dungeon_prepare_stage`, `dungeon_advance_travel`, `dungeon_save_progress`, `dungeon_restore_save`, `dungeon_after_battle_win` |
| `dungeon_control.cpp` | 162 | `dungeon_move_selection`, `dungeon_pick_chest_options`, `dungeon_apply_chest_item` |
| `dungeon_action.cpp` | 325 | `dungeon_confirm_action`, `dungeon_tick`, `dungeon_use_best_item`, `dungeon_start_battle` |
| `dungeon_effect.cpp` | 127 | `dungeon_register_*_damage`, `dungeon_update_visual_effects` |
| `scr_dungeon_game.cpp` | 532 | `dungeon_draw_*`, `view_scr_dungeon_game`, `scr_dungeon_game_handle` |

## 2. State chung

`dungeon_runtime.h`. Một struct duy nhất, là nguồn chân lý:

```c
typedef struct {
    uint8_t level;                  /* 1..5 */
    uint8_t stage;                  /* màn hiện tại trong level */
    uint8_t total_stages;           /* 4/5/6/7/8 tuỳ level */
    uint8_t current_view;           /* TRAVEL / CHEST / MESSAGE / BATTLE */
    uint8_t current_monster;
    uint8_t battle_turn;            /* đếm lượt, dùng cho pattern của quái */
    uint8_t battle_phase;           /* pha animation trong lượt đánh */
    uint8_t battle_wait_ticks;      /* đếm ngược cho pha hiện tại */
    uint8_t travel_progress;        /* 0..100 */
    uint8_t selected_action;        /* ATK/ITEM/DEF/SKL/ESC */
    uint8_t defend_active;          /* đỡ đòn, giảm nửa sát thương kế */
    uint8_t poison_turns, burn_turns, curse_turns;
    uint8_t enemy_poison_turns, enemy_poison_damage;
    uint8_t monster_dodge_ready;    /* sói né đòn */
    uint8_t inventory[DUNGEON_ITEM_COUNT];
    uint8_t chest_options[3];
    int16_t player_hp, player_max_hp, player_atk, player_def;
    int16_t monster_hp, monster_max_hp, monster_dmg, monster_armor;
    int16_t pending_attack_damage;  /* sát thương chờ áp ở pha APPLY */
    uint8_t pending_attack_hit;
    /* ...popup + shake ticks... */
    char line_1[22], line_2[22], line_3[22];   /* nội dung màn MESSAGE */
} dungeon_runtime_t;

extern dungeon_runtime_t dungeon_runtime;
```

Không có mutex nào cả. Mà cũng không cần.

AK dispatch từng task một trên một stack, không thằng nào cắt ngang thằng nào. Ghi chú này nằm ngay đầu `dungeon_runtime.h`.

Bảng dữ liệu tĩnh:

```c
const uint8_t dungeon_stage_counts[5] = {4, 5, 6, 7, 8};
const uint8_t dungeon_monster_table[5][8] = { ... };   // level x stage -> loại quái
const char* dungeon_monster_name[]  = {"SLIME","GOBLIN","WOLF","GORILLA","DRAGON","EYE WATCHER"};
const char* dungeon_action_name[]   = {"ATK","ITEM","DEF","SKL","ESC"};
const char* dungeon_item_name[]     = {"Sword","Shield","Healing","Bomb","Antidote","Purify","Poison"};
```

## 3. Một nhịp tick chạy sao

Timer 100 ms bắn `DUNGEON_TIME_TICK` cho task display:

```c
/* app.h */
#define DUNGEON_TIME_TICK_INTERVAL  (100)

/* dungeon_start_tick_timer() trong scr_dungeon_game.cpp */
timer_set(AC_TASK_DISPLAY_ID, DUNGEON_TIME_TICK, DUNGEON_TIME_TICK_INTERVAL, TIMER_PERIODIC);
```

Màn hình nhận rồi chia việc ra:

```c
case DUNGEON_TIME_TICK: {
    /* Thứ tự quan trọng: cả 5 task game đều ở TASK_PRI_LEVEL_4 nên chạy
     * FIFO theo đúng thứ tự post. Hiệu ứng phải tàn trước khi máy lượt
     * chạy tiếp, đúng như hồi dungeon_tick() tự làm cả hai. */
    task_post_pure_msg(DUNGEON_CONTROL_ID, DUNGEON_CONTROL_UPDATE);
    task_post_pure_msg(DUNGEON_EFFECT_ID,  DUNGEON_EFFECT_UPDATE);
    task_post_pure_msg(DUNGEON_LANE_ID,    DUNGEON_LANE_LEVEL_UP);
    task_post_pure_msg(DUNGEON_ACTION_ID,  DUNGEON_ACTION_RUN);
}
```

Vẽ ra thì như vầy:

```raw
timer 100ms
    │
    ▼
DUNGEON_TIME_TICK -> task_display -> scr_dungeon_game_handle
    │
    ├─> DUNGEON_CONTROL_UPDATE ──> dungeon_control: đổi frame animation nhân vật
    ├─> DUNGEON_EFFECT_UPDATE  ──> dungeon_effect : giảm popup/shake ticks
    ├─> DUNGEON_LANE_LEVEL_UP  ──> dungeon_lane   : tiến travel_progress
    └─> DUNGEON_ACTION_RUN     ──> dungeon_action : dungeon_tick(), máy pha đánh
                                        │
                                        └─> có thể post DUNGEON_STATE_RUN
```

Xử lý xong 4 message thì kernel quay lại `scr_mng_render_screen()` để tính chuyện vẽ.

Travel với battle là hai view loại trừ nhau. Nên `dungeon_advance_travel()` và `dungeon_tick()` không bao giờ đụng cùng field trong cùng một tick.

## 4. Máy trạng thái lượt đánh

`battle_phase` đi qua mấy pha này, khai trong `dungeon_runtime.h`:

```c
enum {
    DUNGEON_BATTLE_PHASE_INPUT = 0,             // chờ người chơi chọn
    DUNGEON_BATTLE_PHASE_HERO_ATK_LUNGE,        // hero lao tới
    DUNGEON_BATTLE_PHASE_HERO_ATK_HIT,          // chạm, áp sát thương
    DUNGEON_BATTLE_PHASE_HERO_ATK_APPLY,        // hết đòn hero
    DUNGEON_BATTLE_PHASE_MONSTER_ATK_LUNGE,     // quái lao tới
    DUNGEON_BATTLE_PHASE_MONSTER_ATK_HIT,       // chạm
    DUNGEON_BATTLE_PHASE_MONSTER_ATK_APPLY,     // hết đòn quái
    DUNGEON_BATTLE_PHASE_MONSTER_ATK_RESOLVE,   // chờ dungeon_state trả lời
};
```

Mỗi nhịp `dungeon_tick()` giảm `battle_wait_ticks`. Về 0 thì sang pha kế:

```c
if (dungeon_runtime.battle_wait_ticks > 0) dungeon_runtime.battle_wait_ticks--;

if (dungeon_runtime.battle_wait_ticks == 0) {
    if (phase == HERO_ATK_LUNGE) {
        phase = HERO_ATK_HIT;  wait = DUNGEON_BATTLE_STEP_TICKS;
        if (pending_attack_hit) monster_shake_ticks = DUNGEON_BATTLE_STEP_TICKS;
    }
    else if (phase == HERO_ATK_HIT) {
        if (pending_attack_hit) dungeon_enemy_take_damage_internal(pending_attack_damage, 0);
        if (monster_hp <= 0) { dungeon_after_battle_win(); return; }
        phase = HERO_ATK_APPLY;  wait = DUNGEON_BATTLE_STEP_TICKS;
    }
    ...
}
```

Sát thương thì tính trước, áp sau. `pending_attack_damage` được tính lúc bấm nút, nhưng chỉ trừ máu ở pha `HIT`. Nhờ vậy animation với con số hiện lên mới khớp nhau.

### Bàn giao lượt quái bằng message

Chỗ này RTOS thể hiện rõ nhất.

```c
/* dungeon_action.cpp */
else if (phase == DUNGEON_BATTLE_PHASE_MONSTER_ATK_HIT) {
    /* Lượt của quái thuộc về dungeon_state: nó sở hữu AI từng loại quái
     * và các status effect. Giao việc rồi đỗ ở RESOLVE chờ trả lời. */
    dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_MONSTER_ATK_RESOLVE;
    task_post_pure_msg(DUNGEON_STATE_ID, DUNGEON_STATE_RUN);
}
else if (phase == DUNGEON_BATTLE_PHASE_MONSTER_ATK_RESOLVE) {
    /* đang chờ dungeon_state */
}
```

```c
/* dungeon_state.cpp */
static void dungeon_monster_turn() {
    dungeon_enemy_action();      // AI riêng từng loại quái
    dungeon_status_tick();       // độc, bỏng, nguyền
    dungeon_runtime.battle_turn++;

    if (dungeon_runtime.monster_hp <= 0) {          // độc hạ gục quái
        dungeon_after_battle_win();
        BUZZER_PlayTones(tones_startup);
        return;
    }
    if (dungeon_runtime.player_hp <= 0) {
        /* dungeon_lane sở hữu vòng đời ván nên nó quyết cách kết thúc */
        task_post_pure_msg(DUNGEON_LANE_ID, DUNGEON_LANE_CHECK_GAME_OVER);
        return;
    }

    /* đẩy pha, tức là giải phóng dungeon_action khỏi RESOLVE */
    dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_MONSTER_ATK_APPLY;
    dungeon_runtime.battle_wait_ticks = DUNGEON_BATTLE_STEP_TICKS;
    dungeon_save_progress();
}
```

Tại sao phải đẻ ra pha `RESOLVE` riêng?

Vì `task_post` chỉ xếp hàng thôi. Message được xử lý sau khi handler hiện tại return. Nếu cứ để nguyên pha `MONSTER_ATK_HIT` thì một tick tới sớm sẽ post yêu cầu lượt quái lần thứ hai.

Pha `RESOLVE` mà `dungeon_tick()` cố tình không làm gì chính là cái chốt chặn đó.

Một lượt đầy đủ nhìn như vầy:

```raw
người chơi bấm MODE
    │
    ▼
DUNGEON_ACTION_SHOOT -> dungeon_confirm_action()
    │  tính pending_attack_damage
    │  phase = HERO_ATK_LUNGE
    │
    ├── tick ──> HERO_ATK_HIT   ──> trừ máu quái, rung hình
    ├── tick ──> HERO_ATK_APPLY ──> dọn pending, xếp lượt quái
    ├── tick ──> MONSTER_ATK_LUNGE
    ├── tick ──> MONSTER_ATK_HIT
    │              │
    │              └──> post DUNGEON_STATE_RUN ──┐
    │                   phase = RESOLVE          │
    │                                            ▼
    │                                    dungeon_state:
    │                                      enemy_action()
    │                                      status_tick()
    │                                      battle_turn++
    │                                      phase = MONSTER_ATK_APPLY
    │
    └── tick ──> MONSTER_ATK_APPLY ──> phase = INPUT, chờ lượt mới
```

## 5. AI từng loại quái

`dungeon_state.cpp`, hàm `dungeon_enemy_action()`. Mỗi quái có chiêu riêng theo chu kỳ lượt:

```c
uint8_t dungeon_turn_matches(uint8_t turn, uint8_t first, uint8_t step) {
    return ((turn >= first) && (((turn - first) % step) == 0));
}
```

| Quái | HP | DMG | Giáp | Chiêu riêng |
|---|---|---|---|---|
| SLIME | 30 + (lv−1)×3 | 5 | 1 | Lượt chẵn thì tự hồi 5 máu |
| GOBLIN | 50 | 10 + (lv−1)×2 | 2 | Lượt 2, 5, 8... gây độc 3 lượt |
| WOLF | 70 | 30 | 1 | Lượt 3, 7, 11... sẵn sàng né đòn |
| GORILLA | 90 | 40 | 4 + lv×2 | Lượt 2, 5, 8... cộng 5 giáp |
| DRAGON | 150 | 50 | lv≥3 ? (lv−2)×7 : 7 | Lượt 3, 7, 11... trừ 10 máu, bỏng 3 lượt |
| EYE WATCHER | 200 | 60 | 12 | Lượt 3, 6, 9... nguyền 3 lượt |

Công thức sát thương:

```c
int16_t damage = monster_dmg - (player_def / 2);
if (defend_active) { damage /= 2; defend_active = 0; }
damage = dungeon_max_int16(damage, 1);        // luôn ăn tối thiểu 1
```

## 6. Lưu game với chơi tiếp

`dungeon_lane.cpp` gói dữ liệu, `app_eeprom.cpp` lo phần đọc ghi và kiểm tra.

```c
#define DUNGEON_SAVE_MAGIC  (0x44554E32UL)    /* "DUN2" */
```

`dungeon_save_progress()` đóng gói khoảng 30 field của runtime vào `dungeon_game_save_t` rồi gọi `dungeon_save_write()`. Nó được gọi ở nhiều mốc trong lượt đánh, nên mất điện giữa trận vẫn chơi tiếp được.

### Ba chỗ từng làm treo máy khi mất điện

Chỗ này từng có bug thật: đang chơi mà mất điện, bật lại bấm Continue thì màn hình đứng im, bấm nút gì cũng không nhúc nhích. Ba nguyên nhân, cả ba đều đã vá.

**Một — biến chỉ sống trong RAM.** `dungeon_message_next` cho biết bấm MODE ở màn thông báo thì đi đâu tiếp. Nó **không** nằm trong bản save. Mất điện lúc màn hình đang hiện "Monster appears / MODE TO BATTLE" thì bản save ghi `current_view = MESSAGE`, nhưng bật lại `dungeon_message_next` là 0 (`DUNGEON_NEXT_NONE`). Mà trong `dungeon_confirm_action()`:

```c
if (dungeon_runtime.current_view == DUNGEON_VIEW_MESSAGE) {
    if      (dungeon_message_next == DUNGEON_NEXT_BATTLE) { ... }
    else if (dungeon_message_next == DUNGEON_NEXT_STAGE)  { ... }
    /* ... không có nhánh nào cho DUNGEON_NEXT_NONE ... */
    return;                     /* <- rơi thẳng xuống đây, không làm gì cả */
}
```

Không nhánh nào khớp, hàm return, màn hình đứng im vĩnh viễn. Mà màn thông báo lại là chỗ hay đứng nhất: trước mỗi trận đánh, sau mỗi lần nhặt rương. Vá bằng cách thêm `message_next` vào bản save.

**Hai — trạng thái đúng cú pháp nhưng đi không được.** Ví dụ `current_view = TRAVEL`, `travel_progress = 100`, `support_pending = 0`. Hero đứng ở cuối đường, mà `dungeon_advance_travel()` thì:

```c
if (dungeon_runtime.travel_progress >= 100) {
    dungeon_runtime.travel_progress = 100;
    if (dungeon_runtime.support_pending) { ... }   /* = 0 nên không vào */
}
```

Kẹp ở 100 rồi thôi, không ai đẩy sang rương hay sang trận nữa. Tương tự còn: `CHEST` mà `support_event = 0`, `BATTLE` mà `monster_max_hp = 0`. Vá bằng `dungeon_sanitize_restored_state()` — nạp save xong thì rà lại, chỗ nào đi không được thì kéo về trạng thái đi được. Hàm này ưu tiên "luôn chơi tiếp được" hơn là "khôi phục chính xác từng chi tiết".

**Ba — ghi EEPROM dở dang.** Bản ghi save dài 56 byte. Mất điện giữa lúc ghi thì đầu là bản mới, đuôi vẫn là bản cũ. `magic` nằm ở 4 byte đầu nên gần như chắc chắn đã ghi xong, `valid` cũng vậy. Luật kiểm tra cũ chỉ xem hai thứ đó:

```c
return ((save_data.magic == DUNGEON_SAVE_MAGIC) && (save_data.valid == 1));
```

nên bản ghi nửa nạc nửa mỡ vẫn lọt. Chạy thử cắt ở đủ 47 vị trí: **luật cũ nhận nhầm 47/47**. Vá bằng cách thêm `check_sum` vào cuối bản ghi, tính đúng như mấy bản ghi khác trong `app_eeprom.cpp`. Sau khi vá: nhận nhầm 0/47.

Một cái bẫy nhỏ khi viết đoạn checksum này: **phải dùng `offsetof`, không được lấy `sizeof - 1`.**

```c
#define DUNGEON_SAVE_CHECKSUM_SIZE  ((uint32_t)offsetof(dungeon_game_save_t, check_sum))
```

Struct căn theo 4 byte vì có `uint32_t magic` ở đầu, nên `check_sum` nằm ở offset 52 mà `sizeof` lại là 56. Lấy `sizeof - 1 = 55` thì phép cộng dồn nuốt luôn cả ô `check_sum` lẫn ba byte đệm, đọc lại không bao giờ khớp, và triệu chứng là **menu không bao giờ hiện Continue nữa** — sai theo kiểu êm ru, rất khó nhìn ra.

`DUNGEON_SAVE_MAGIC` đổi từ `"DUNG"` sang `"DUN2"` để bản save đời cũ (không có `message_next`, không có `check_sum`) bị bỏ qua đúng một lần thay vì cố đọc rồi đoán mò.

Có 4 kiểu vào game:

```c
void dungeon_prepare_continue()             { dungeon_start_mode = DUNGEON_START_CONTINUE; }
void dungeon_prepare_new_game()             { dungeon_start_mode = DUNGEON_START_NEW_GAME;
                                              dungeon_selected_level = 1; dungeon_clear_save(); }
void dungeon_prepare_level(uint8_t level)   { dungeon_start_mode = DUNGEON_START_LEVEL; ... }
void dungeon_prepare_creator_mode(uint8_t l){ dungeon_start_mode = DUNGEON_START_CREATOR; ... }
```

Chế độ creator đặt `dungeon_persist_enabled = 0`, chơi thử mà không ghi đè save thật.

Có một cái bẫy ở đây, tránh được rồi. `DUNGEON_STATE_SETUP` cố tình **không** gọi `dungeon_set_monster_stats()`.

Quái được lập ở `dungeon_prepare_stage()` khi vào màn mới, hoặc ở `dungeon_restore_save()` khi chơi tiếp. Lập lại ở SETUP thì mỗi lần continue con quái đang bị thương sẽ hồi đầy máu. Comment cảnh báo nằm ngay trong code.

## 7. Hiệu ứng hình ảnh

`dungeon_effect.cpp` lo popup số damage với rung hình:

```c
#define DUNGEON_SHAKE_TICKS  (6)
#define DUNGEON_POPUP_TICKS  (24)

void dungeon_register_player_damage(int16_t amount) {
    if (amount <= 0) return;
    dungeon_runtime.player_hp_popup_value = amount;
    dungeon_runtime.player_hp_popup_ticks = DUNGEON_POPUP_TICKS;
    dungeon_runtime.player_shake_ticks    = DUNGEON_SHAKE_TICKS;
}

void dungeon_update_visual_effects() {
    if (dungeon_runtime.player_hp_popup_ticks > 0) dungeon_runtime.player_hp_popup_ticks--;
    ...
}
```

Handler áp đúng điều kiện `GAME_PLAY` mà `dungeon_tick()` từng áp trước khi gọi:

```c
case DUNGEON_EFFECT_UPDATE:
    if (dungeon_game_state == GAME_PLAY) dungeon_update_visual_effects();
    break;
```

## 8. Phần vẽ

`scr_dungeon_game.cpp` giờ chỉ còn view. Bốn hàm vẽ, chọn theo `current_view`:

```c
static void view_scr_dungeon_game() {
    switch (dungeon_runtime.current_view) {
    case DUNGEON_VIEW_TRAVEL:  dungeon_draw_travel();  break;
    case DUNGEON_VIEW_CHEST:   dungeon_draw_chest();   break;
    case DUNGEON_VIEW_MESSAGE: dungeon_draw_message(); break;
    case DUNGEON_VIEW_BATTLE:  dungeon_draw_battle();  break;
    }
}
```

Bitmap tra qua struct:

```c
typedef struct {
    const uint8_t* data;
    uint8_t width, height;
} dungeon_bitmap_t;

static dungeon_bitmap_t dungeon_monster_bitmap(uint8_t monster);
static dungeon_bitmap_t dungeon_item_bitmap(uint8_t item);
```

Dữ liệu bitmap nằm ở `screens_bitmap.cpp`. Ảnh gốc ở `resources/images/bitmaps/`.

## 9. Ba signal để rỗng, cố ý

`DUNGEON_STATE_SETUP`, `DUNGEON_STATE_RESET`, `DUNGEON_STATE_DETONATOR` không làm gì cả. Cố ý, và trong code có comment giải thích.

Lý do thật là game này theo lượt.

Khác game archery mẫu, bên đó mũi tên bay, thiên thạch rơi, vụ nổ chạy, ba thứ độc lập cần tick song song mỗi nhịp. Còn ở đây vòng đời của quái do `dungeon_lane` quản, nên `dungeon_state` không có việc gì làm lúc setup hay reset.

Ép việc cho chúng thì sinh lỗi thật, đúng cái bẫy ở mục 6. Để rỗng kèm giải thích vẫn hơn làm bừa.
