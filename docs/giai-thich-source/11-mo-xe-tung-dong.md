# 11 — Mổ xẻ từng dòng

Năm hàm cốt lõi, đọc kỹ từng dòng. Hiểu được năm hàm này là hiểu cách cả hệ thống chạy.

---

## 1. `task_post()` — đưa message vào hàng đợi

`ak/src/task.c:98`. Đây là hàm được gọi nhiều nhất trong toàn bộ project.

```c
void task_post(task_id_t des_task_id, ak_msg_t* msg) {
    tcb_t* t_tcb;
```

`des_task_id` là ID task đích. `msg` là message đã cấp phát từ pool.

```c
    if (des_task_id >= task_table_size) {
        FATAL("TK", 0x02);
    }
```

Chặn ID vượt bảng. `task_table_size` đếm được lúc `task_create()` chạy, nó đếm tới khi gặp `AK_TASK_EOT_ID`.

Post nhầm ID là `FATAL` ngay, không âm thầm ghi bậy. Mã `TK 0x02` nhớ lấy, gặp trên UART là biết ngay lỗi gì.

```c
    t_tcb = &task_pri_queue[task_table[des_task_id].pri - 1];
```

Dòng này làm hai việc.

`task_table[des_task_id].pri` lấy mức ưu tiên của task đích. Rồi `- 1` để đổi từ mức (1..7) sang index mảng (0..6).

Đây chính là lý do **mức ưu tiên phải từ 1 trở lên**. Đặt `TASK_PRI_LEVEL_0` là index `-1`, ghi đè bộ nhớ trước mảng.

```c
    ENTRY_CRITICAL();
```

Từ đây tắt ngắt. Bắt buộc, vì hàm này chạy được cả từ task lẫn từ ngắt SysTick. Không khoá thì hai bên cùng sửa hàng đợi một lúc, danh sách liên kết đứt.

```c
    msg->next = AK_MSG_NULL;
    msg->des_task_id = des_task_id;
```

Message sắp vào cuối hàng nên `next` phải NULL. Ghi luôn ID đích vào message, để lát nữa scheduler biết gọi hàm nào.

```c
    if (t_tcb->qtail == AK_MSG_NULL) {
        /* hàng đợi đang rỗng */
        t_tcb->qtail = msg;
        t_tcb->qhead = msg;

        task_ready |= t_tcb->mask;
    }
```

Hàng rỗng thì message này vừa là đầu vừa là cuối.

Dòng `task_ready |= t_tcb->mask` là mấu chốt. `mask` được gán lúc `task_init()`:

```c
t_tcb->mask = (1 << (pri - 1));
```

Mức 4 thì mask là `0b00001000`. Bật bit đó lên nghĩa là báo cho scheduler "mức 4 có việc rồi".

```c
    else {
        /* hàng đã có message */
        t_tcb->qtail->next = msg;
        t_tcb->qtail = msg;
    }
```

Hàng đã có thì nối vào cuối. Khỏi bật `task_ready` vì nó đã bật sẵn.

Đây chính là chỗ đảm bảo **FIFO**. Message post trước nằm trước. Cùng mức ưu tiên thì chạy đúng thứ tự post.

```c
    EXIT_CRITICAL();
}
```

Mở ngắt lại. Xong.

Để ý là hàm này **không gọi task nào cả**. Nó chỉ xếp hàng rồi về. Việc gọi để scheduler lo.

---

## 2. `task_sheduler()` — vòng lặp chạy task

`ak/src/task.c:352`. Trái tim của kernel.

```c
void task_sheduler() {
    uint8_t t_task_new;

    ENTRY_CRITICAL();

    uint8_t t_task_current = task_current;
```

Chụp lại mức ưu tiên đang chạy. Ban đầu `task_current` bằng 0.

```c
    while ((t_task_new = LOG2LKUP(task_ready)) > t_task_current) {
```

Dòng quan trọng nhất cả kernel.

`LOG2LKUP(x)` là `32 - __builtin_clz(x)`. `__builtin_clz` đếm số bit 0 ở đầu (count leading zeros). CPU ARM có lệnh `CLZ` riêng nên cái này chỉ tốn một chu kỳ.

Kết quả là vị trí bit 1 cao nhất, tức mức ưu tiên cao nhất đang có việc.

Ví dụ:

```raw
task_ready = 0b00010100
             bit 4 và bit 2 đang bật
             -> mức 5 và mức 3 có việc
__builtin_clz = 27  (27 số 0 ở đầu của số 32 bit)
LOG2LKUP      = 32 - 27 = 5
-> chạy mức 5 trước
```

`task_ready = 0` thì `LOG2LKUP` ra 0, không lớn hơn `t_task_current` (cũng 0), vòng lặp thoát. Hết việc thì về.

```c
        tcb_t* t_tcb = &task_pri_queue[t_task_new - 1];

        ak_msg_t* t_msg = t_tcb->qhead;
        t_tcb->qhead = t_msg->next;
```

Lấy message đầu hàng, dời `qhead` sang cái kế.

```c
        if (t_msg->next == AK_MSG_NULL) {
            t_tcb->qtail = AK_MSG_NULL;
            task_ready &= ~t_tcb->mask;
        }
```

Vừa lấy cái cuối cùng thì hàng rỗng. Tắt bit trong `task_ready`.

Không tắt thì vòng `while` lặp vô tận vì `LOG2LKUP` vẫn trả về mức đó.

```c
        task_current = t_task_new;
```

Ghi lại mức đang chạy. Cái này để `task_self()` và mấy hàm debug biết đang ở đâu.

```c
        memcpy(&current_task_info, &task_table[t_msg->des_task_id], sizeof(task_t));
        memcpy(&current_active_object, t_msg, sizeof(ak_msg_t));

        current_task_id = t_msg->if_des_task_id;
```

Chép thông tin task và message hiện tại ra biến toàn cục. Dùng cho debug và cho `get_current_active_object()`.

```c
        EXIT_CRITICAL();

        task_table[t_msg->des_task_id].task(t_msg);

        ENTRY_CRITICAL();
```

Đây là chỗ **task thật sự chạy**.

Để ý ngắt được mở trước khi gọi. Đúng vậy, task chạy với ngắt bật. Nên trong lúc task chạy, SysTick vẫn tick, nút vẫn quét được, message mới vẫn post được.

Cái không xảy ra là: không có task nào khác chen vào giữa. Message mới chỉ nằm chờ trong hàng đợi thôi.

```c
        msg_free(t_msg);
    }
```

Trả message về pool. `msg_free` kiểm ref count, chỉ trả thật khi về 0.

```c
    task_current = t_task_current;

    current_task_id = AK_TASK_IDLE_ID;

    EXIT_CRITICAL();
}
```

Khôi phục `task_current` về giá trị lúc vào. Đánh dấu đang rảnh. Về.

### Cả vòng nhìn lại

```raw
task_ready = 0b00010000        (mức 5 có việc)
  │
  ├─ LOG2LKUP -> 5, lớn hơn 0 -> vào vòng
  │    lấy message của mức 5
  │    hàng rỗng -> task_ready = 0b00000000
  │    gọi task
  │       └── task này post cho mức 4 -> task_ready = 0b00001000
  │    msg_free
  │
  ├─ LOG2LKUP -> 4, lớn hơn 0 -> vào vòng tiếp
  │    lấy message của mức 4
  │    gọi task
  │    msg_free
  │
  └─ LOG2LKUP -> 0, không lớn hơn 0 -> thoát
```

Vòng lặp chạy tới khi **sạch hết** message. Rồi `task_run()` mới gọi `task_polling_run()`.

---

## 3. `task_timer_tick()` — hàm biến thời gian thành signal

`ak/src/timer.c`. Chạy ở mức ưu tiên 7, cao nhất.

```c
void task_timer_tick(ak_msg_t* msg) {
    ak_msg_t* timer_msg;
    ak_timer_t* timer_list;
    ak_timer_t* timer_del = TIMER_MSG_NULL;
    uint32_t temp_counter;
    uint32_t irq_counter;

    ENTRY_CRITICAL();

    timer_list = timer_list_head;

    irq_counter = ak_timer_payload_irq.counter;

    ak_timer_payload_irq.counter = 0;
    ak_timer_payload_irq.enable_post_msg = AK_ENABLE;

    EXIT_CRITICAL();
```

Ba dòng giữa là phần tinh tế nhất.

`irq_counter` lấy tổng số mili giây đã trôi từ lần xử lý trước. Không phải 1, mà là **tổng cộng dồn**.

Vì sao. Nhìn lại phía ngắt:

```c
void timer_tick(uint32_t t) {
    if (timer_list_head != TIMER_MSG_NULL) {
        ak_timer_payload_irq.counter += t;                    /* cộng dồn */
        if (ak_timer_payload_irq.enable_post_msg == AK_ENABLE) {
            ak_timer_payload_irq.enable_post_msg = AK_DISABLE; /* khoá */
            ...post TIMER_TICK...
        }
    }
}
```

Ngắt chạy mỗi 1 ms và luôn cộng counter. Nhưng chỉ post message khi cờ đang mở, và post xong thì khoá cờ lại.

Nên nếu hệ thống bận 5 ms, ngắt chạy 5 lần, counter lên 5, mà chỉ có **1** message trong hàng đợi.

Rồi `task_timer_tick` lấy trọn 5 ms đó ra và mở khoá cho lần sau.

Không có cơ chế này thì hệ thống bận một chút là pool message ngập ngay.

```c
    switch (msg->sig) {
    case TIMER_TICK:
        while (timer_list != TIMER_MSG_NULL) {

            ENTRY_CRITICAL();

            if (irq_counter < timer_list->counter) {
                timer_list->counter -= irq_counter;
            }
            else {
                timer_list->counter = 0;
            }

            temp_counter = timer_list->counter;

            EXIT_CRITICAL();
```

Duyệt danh sách timer, trừ **cả cụm** `irq_counter` một lần.

Cái `if/else` tránh trừ âm. `counter` là `uint32_t`, trừ quá là quay vòng thành số cực lớn, timer không bao giờ bắn.

Chép ra `temp_counter` để đọc ngoài vùng khoá. Vì đoạn dưới có gọi `get_pure_msg()` và `task_post()`, hai hàm này tự khoá riêng, giữ khoá quá lâu không tốt.

```c
            if (temp_counter == 0) {

                timer_msg = get_pure_msg();
                set_msg_sig(timer_msg, timer_list->sig);
                task_post(timer_list->des_task_id, timer_msg);
```

Hết giờ thì lấy một message từ pool, gán signal đã đăng ký, post cho task đích.

Đây chính là chỗ **thời gian biến thành signal**. Mọi thứ định kỳ trong project đều đi qua ba dòng này.

```c
                ENTRY_CRITICAL();

                if (timer_list->period) {
                    timer_list->counter = timer_list->period;
                }
                else {
                    timer_del = timer_list;
                }

                EXIT_CRITICAL();
            }
```

`period` khác 0 nghĩa là `TIMER_PERIODIC`, nạp lại counter để chạy tiếp.

`period` bằng 0 là `TIMER_ONE_SHOT`, đánh dấu để xoá.

```c
            timer_list = timer_list->next;

            if (timer_del) {
                timer_remove_msg(timer_del->des_task_id, timer_del->sig);
                timer_del = TIMER_MSG_NULL;
            }
        }
        break;
```

Để ý thứ tự: **dời con trỏ trước, xoá sau**.

Xoá trước thì `timer_list->next` đọc vào vùng nhớ vừa trả về pool. Lỗi use-after-free kinh điển. Ở đây tránh được bằng cách dời con trỏ đi rồi mới xoá.

---

## 4. `dungeon_tick()` — nhịp đập của game

`app/game/dungeon_game/dungeon_action.cpp:221`. Chạy mỗi 100 ms qua `DUNGEON_ACTION_RUN`.

```c
void dungeon_tick() {
    if (dungeon_game_state != GAME_PLAY) {
        return;
    }
```

Chốt chặn đầu tiên. Game over hay chưa vào game thì thôi.

Không có dòng này thì máy trạng thái vẫn chạy sau khi thua, HP tiếp tục trừ, cực khó hiểu chuyện gì đang xảy ra.

```c
    /* Popup / shake decay là việc của dungeon_effect (DUNGEON_EFFECT_UPDATE),
     * màn hình post cái đó ngay trước DUNGEON_ACTION_RUN. */

    if (dungeon_runtime.current_view == DUNGEON_VIEW_BATTLE) {
        if (dungeon_runtime.battle_phase != DUNGEON_BATTLE_PHASE_INPUT) {
```

Chỉ chạy máy pha khi đang trong trận **và** không ở pha chờ người chơi.

Pha `INPUT` nghĩa là đang chờ bấm nút. Tick không làm gì cả, người chơi ngồi nghĩ bao lâu cũng được.

```c
            if (dungeon_runtime.battle_wait_ticks > 0) {
                dungeon_runtime.battle_wait_ticks--;
            }

            if (dungeon_runtime.battle_wait_ticks == 0) {
```

Đếm ngược. Về 0 mới chuyển pha.

Đây là cách làm animation mà không cần `delay()`. Mỗi pha đặt `battle_wait_ticks = 5`, tức 5 nhịp × 100 ms = 500 ms, rồi tick tự đếm xuống.

```c
                if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_HERO_ATK_LUNGE) {
                    dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_HERO_ATK_HIT;
                    dungeon_runtime.battle_wait_ticks = DUNGEON_BATTLE_STEP_TICKS;
                    if (dungeon_runtime.pending_attack_hit) {
                        dungeon_runtime.monster_shake_ticks = DUNGEON_BATTLE_STEP_TICKS;
                    }
                    dungeon_save_progress();
                }
```

Pha lao tới xong thì sang pha chạm, hẹn 5 tick nữa, và bật rung hình nếu đòn này trúng.

`dungeon_save_progress()` gọi ở gần như mọi chuyển pha. Nghĩa là mất điện giữa trận vẫn khôi phục đúng chỗ.

Đánh đổi là ghi EEPROM khá nhiều. EEPROM nội chịu khoảng 10⁵ lần ghi, chơi nhiều thì cũng đáng để ý.

```c
                else if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_HERO_ATK_HIT) {
                    if (dungeon_runtime.pending_attack_hit) {
                        dungeon_enemy_take_damage_internal(dungeon_runtime.pending_attack_damage, 0);
                    }

                    if (dungeon_runtime.monster_hp <= 0) {
                        dungeon_after_battle_win();
                        BUZZER_PlayTones(tones_startup);
                        return;
                    }

                    dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_HERO_ATK_APPLY;
                    dungeon_runtime.battle_wait_ticks = DUNGEON_BATTLE_STEP_TICKS;
                    dungeon_save_progress();
                }
```

Pha chạm mới thật sự trừ máu.

Sát thương đã tính từ lúc bấm nút và cất ở `pending_attack_damage`. Tới đây mới áp.

Tách ra như vậy để con số hiện lên khớp với lúc kiếm chạm vào quái. Tính và áp cùng lúc bấm nút thì máu tụt trước khi animation chạy.

Tham số thứ hai của `dungeon_enemy_take_damage_internal(dmg, 0)` là `trigger_shake`. Truyền 0 vì rung hình đã bật từ pha trước rồi.

```c
                else if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_MONSTER_ATK_HIT) {
                    dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_MONSTER_ATK_RESOLVE;
                    task_post_pure_msg(DUNGEON_STATE_ID, DUNGEON_STATE_RUN);
                }
                else if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_MONSTER_ATK_RESOLVE) {
                    /* đang chờ dungeon_state */
                }
```

Đây là chỗ giao việc sang task khác.

Đổi pha **trước** rồi mới post. Thứ tự này quan trọng.

Post trước rồi mới đổi pha thì cũng chạy đúng thôi (vì message xử lý sau khi hàm này return), nhưng đổi trước đọc rõ ràng hơn: pha là cái khoá, post là cái yêu cầu.

Nhánh `RESOLVE` rỗng chính là chốt chặn. Tick tới trong lúc chờ thì rơi vào đây, không làm gì, không post trùng.

```c
        }
        return;
    }
```

Đang trong trận thì return luôn. Phần travel bên dưới không chạy.

```c
    /* Travel là việc của dungeon_lane, chạy bằng DUNGEON_LANE_LEVEL_UP.
     * Battle và travel là hai view loại trừ nhau nên hai task không bao giờ
     * đụng cùng field trong một tick. */
}
```

Comment cuối giải thích vì sao tách travel sang task khác mà không sợ tranh chấp.

---

## 5. `dungeon_confirm_action()` — xử lý nút MODE

`app/game/dungeon_game/dungeon_action.cpp:111`. Đây là hàm phức tạp nhất về mặt luật chơi.

```c
void dungeon_confirm_action() {
    if (dungeon_game_state != GAME_PLAY) {
        return;
    }
```

Lại chốt chặn quen thuộc.

```c
    if (dungeon_runtime.current_view == DUNGEON_VIEW_MESSAGE) {
        if (dungeon_message_next == DUNGEON_NEXT_BATTLE) {
            dungeon_start_battle();
        }
        else if (dungeon_message_next == DUNGEON_NEXT_STAGE) {
            dungeon_advance_stage();
        }
        else if (dungeon_message_next == DUNGEON_NEXT_WIN) {
            dungeon_finish_game(DUNGEON_OUTCOME_WIN);
        }
        else if (dungeon_message_next == DUNGEON_NEXT_LOSE) {
            dungeon_finish_game(DUNGEON_OUTCOME_LOSE);
        }
        else if (dungeon_message_next == DUNGEON_NEXT_TRAVEL) {
            dungeon_runtime.current_view = DUNGEON_VIEW_TRAVEL;
        }
        else if (dungeon_message_next == DUNGEON_NEXT_RETURN) {
            dungeon_runtime.current_view = DUNGEON_VIEW_BATTLE;
        }
        BUZZER_PlayTones(tones_cc);
        return;
    }
```

Đang hiện màn thông báo thì nút MODE nghĩa là "đã đọc xong".

`dungeon_message_next` được đặt lúc gọi `dungeon_set_message(line1, line2, line3, next)`. Nó nói cho hàm này biết bấm xong thì đi đâu.

Kiểu như một máy trạng thái nhỏ: thông báo mang theo sẵn đích đến của nó.

```c
    if (dungeon_runtime.current_view == DUNGEON_VIEW_CHEST) {
        dungeon_apply_chest_item(dungeon_runtime.chest_options[dungeon_runtime.selected_support_item]);
        BUZZER_PlayTones(tones_cc);
        return;
    }
```

Đang mở rương thì nút MODE là chọn món đang trỏ.

`chest_options[3]` chứa 3 item được random. `selected_support_item` là chỉ số đang chọn, do nút UP/DOWN đổi.

```c
    if (dungeon_runtime.current_view != DUNGEON_VIEW_BATTLE) {
        BUZZER_PlayTones(tones_3beep);
        return;
    }

    if (dungeon_runtime.battle_phase != DUNGEON_BATTLE_PHASE_INPUT) {
        BUZZER_PlayTones(tones_3beep);
        return;
    }
```

Hai lần từ chối.

Không ở trong trận thì không có gì để xác nhận. Đang giữa animation đánh cũng không.

`tones_3beep` là âm báo lỗi. Người chơi nghe là biết bấm không ăn, thay vì tưởng máy đơ.

Đây cũng là chỗ chặn spam nút. Bấm 10 lần trong lúc animation chạy thì 10 lần đều rơi vào đây.

```c
    switch (dungeon_runtime.selected_action) {
    case DUNGEON_ACTION_ATTACK:
        dungeon_runtime.defend_icon_active = 0;
        dungeon_runtime.pending_attack_damage = dungeon_player_damage(0);
        dungeon_runtime.pending_attack_hit = 1;
```

Tính sát thương ngay bây giờ, cất vào `pending_*`, chờ pha `HIT` mới áp.

`dungeon_player_damage(0)` với tham số 0 nghĩa là đòn thường. Truyền 1 là dùng skill.

```c
        if ((dungeon_runtime.current_monster == DUNGEON_MONSTER_WOLF) &&
            (dungeon_runtime.monster_dodge_ready != 0)) {
            dungeon_runtime.monster_dodge_ready = 0;
            dungeon_runtime.pending_attack_hit = 0;
            dungeon_runtime.pending_attack_damage = 0;
        }
```

Chiêu né của sói. Cờ `monster_dodge_ready` được `dungeon_enemy_action()` bật ở lượt 3, 7, 11...

Né xong thì tắt cờ. Chỉ né một lần.

Để ý là animation **vẫn chạy đủ**, chỉ có `pending_attack_hit = 0` nên tới pha `HIT` không trừ máu. Người chơi thấy hero lao tới, đánh trượt. Đúng cảm giác né.

```c
        dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_HERO_ATK_LUNGE;
        dungeon_runtime.battle_wait_ticks = DUNGEON_BATTLE_STEP_TICKS;
        dungeon_runtime.monster_shake_ticks = 0;
        dungeon_runtime.player_shake_ticks = 0;
        dungeon_save_progress();
        BUZZER_PlayTones(tones_cc);
        return;
```

Khởi động máy pha. Xoá rung hình cũ để không dính từ lượt trước.

Rồi `return` luôn. Nhánh ATTACK không đi xuống dưới, vì lượt quái sẽ tự tới qua máy pha.

```c
    case DUNGEON_ACTION_ITEM:
        dungeon_runtime.defend_icon_active = 0;
        if (dungeon_use_best_item() == 0) {
            dungeon_set_message("No item ready", "Try another action", "MODE TO RETURN", DUNGEON_NEXT_RETURN);
            BUZZER_PlayTones(tones_3beep);
            return;
        }
        break;
```

Túi rỗng thì hiện thông báo rồi `return`, **không mất lượt**.

`DUNGEON_NEXT_RETURN` đưa người chơi về lại màn trận đấu.

Dùng được item thì `break`, chạy tiếp xuống dưới, tức là mất lượt.

```c
    case DUNGEON_ACTION_DEFEND:
        dungeon_runtime.defend_active = 1;
        dungeon_runtime.defend_icon_active = 1;
        break;
```

Đỡ đòn. Hai cờ khác nhau: `defend_active` là hiệu lực thật, `defend_icon_active` là để vẽ cái khiên.

`defend_active` bị `dungeon_enemy_action()` tắt sau khi giảm nửa sát thương. Chỉ đỡ được một đòn.

```c
    case DUNGEON_ACTION_SKILL:
        dungeon_runtime.defend_icon_active = 0;
        dungeon_enemy_take_damage(dungeon_player_damage(1));
        break;
```

Skill trừ máu **ngay lập tức**, không qua máy pha.

Khác đòn thường. Đòn thường có animation lao tới rồi mới trừ, skill trừ liền.

```c
    default:
        dungeon_runtime.defend_icon_active = 0;
        if ((dungeon_runtime.current_monster == DUNGEON_MONSTER_DRAGON) ||
            (dungeon_runtime.current_monster == DUNGEON_MONSTER_EYE)) {
            dungeon_set_message("Boss blocks path", "Escape failed", "Enemy turn", DUNGEON_NEXT_NONE);
        }
```

Nhánh này là ESCAPE. Gặp boss thì không chạy được.

```c
        else if (((dungeon_runtime.level + dungeon_runtime.stage + dungeon_runtime.battle_turn) % 3) != 0) {
            dungeon_game_score += 5;
            dungeon_set_message("Escape succeeded", "Stage skipped", "MODE NEXT STAGE", DUNGEON_NEXT_STAGE);
            dungeon_save_progress();
            BUZZER_PlayTones(tones_cc);
            return;
        }
```

Tỉ lệ chạy thoát tính bằng `(level + stage + battle_turn) % 3`.

Không dùng random thật. Đây là **giả ngẫu nhiên tất định**: cùng một tình huống thì kết quả luôn giống nhau.

Được cái dễ test và dễ tái hiện lỗi. Mất cái người chơi tinh ý sẽ đoán được.

```c
        else {
            dungeon_set_message("Escape failed", "Enemy attacks", "", DUNGEON_NEXT_NONE);
        }
        break;
    }
```

Chạy trượt thì ăn đòn.

```c
    if (dungeon_runtime.monster_hp <= 0) {
        dungeon_after_battle_win();
        BUZZER_PlayTones(tones_startup);
        return;
    }

    dungeon_queue_enemy_turn();
    dungeon_save_progress();
    BUZZER_PlayTones(tones_cc);
}
```

Phần chung cho mấy nhánh có `break` (ITEM, DEFEND, SKILL, ESCAPE thất bại).

Quái chết thì thắng luôn. Chưa chết thì xếp lượt quái rồi lưu.

### Tóm lại cách hàm này thoát ra

| Nhánh | Thoát bằng | Mất lượt không |
|---|---|---|
| Màn thông báo | `return` | Không |
| Mở rương | `return` | Không |
| Không đúng lúc | `return` + tiếng báo lỗi | Không |
| ATTACK | `return` (máy pha lo tiếp) | Có, qua máy pha |
| ITEM, túi rỗng | `return` | Không |
| ITEM, dùng được | `break` -> xuống cuối | Có |
| DEFEND | `break` -> xuống cuối | Có |
| SKILL | `break` -> xuống cuối | Có |
| ESCAPE thành công | `return` | Không, qua màn luôn |
| ESCAPE thất bại | `break` -> xuống cuối | Có |

Chỗ dễ nhầm nhất khi sửa hàm này: quên mất `return` với `break` khác nhau ra sao. `break` là mất lượt, `return` là không.

---

## 6. Đọc code kiểu này thế nào cho quen

Vài mẹo rút ra từ năm hàm trên.

**Đọc chốt chặn trước.** Mấy dòng `if (...) return;` ở đầu hàm cho biết hàm chạy trong điều kiện nào. Đọc xong mấy dòng đó là hiểu được nửa hàm.

**Tìm `ENTRY_CRITICAL` để biết chỗ nào đụng ngắt.** Có nó nghĩa là dữ liệu đó chia sẻ với ngắt.

**Thấy `task_post` là biết có bàn giao.** Từ đó lần sang task nhận để đọc tiếp.

**Thấy `timer_set` là biết có chờ.** Chờ xong signal gì thì tra ngược lại xem ai xử lý signal đó.

**Cờ hai thằng đi cặp thường là "thật" với "để vẽ".** Như `defend_active` với `defend_icon_active`.

**Biến tên `pending_*` là tính trước áp sau.** Tìm chỗ gán và chỗ dùng, hai chỗ đó cách nhau vài pha.
