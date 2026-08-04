# 08 — RTOS nằm ở đâu, với mấy câu hay bị hỏi

File này gom lại để trả lời câu "đồ án vận dụng RTOS thế nào". Có sẵn đường dẫn tới đúng dòng code để mở ra chỉ.

## 1. Bảng đối chiếu

| Khái niệm RTOS | Nằm ở đâu | File:dòng |
|---|---|---|
| Task / active object | 16 task (13 + 3 tầng link khi bật `IF_LINK_UART_EN`) + 1 polling task | `app/task_list.cpp` |
| Độ ưu tiên | 6 mức đang dùng: 2, 3, 4, 5, 6, 7 | `app/task_list.cpp` |
| Scheduler | Chọn mức cao nhất bằng `LOG2LKUP` | `ak/src/task.c:352` |
| Hàng đợi message | FIFO mỗi mức ưu tiên | `ak/src/task.c:98` |
| IPC | 3 loại message pool | `ak/src/message.c` |
| Software timer | Pool 16, one-shot với periodic | `ak/src/timer.c` |
| Ngắt sang task | SysTick, nút bấm post message | `platform/stm32l/system.c:321`, `app/app_bsp.cpp` |
| Critical section | `ENTRY_CRITICAL` có đếm lồng | `platform/stm32l/platform.h` |
| FSM | 3 tầng giao thức link | `networks/net/link/*.cpp` |
| Watchdog | 2 lớp: phần cứng với phần mềm | `platform/stm32l/sys_cfg.c:415` |
| Bootloader | Vùng riêng, bắt tay qua BSF | `boot/` |

## 2. Ba ví dụ mạnh nhất

### Chờ mà không chặn

`networks/net/link/link_phy.cpp`. Gửi khung xong cần chờ ACK 250 ms:

```c
timer_set(AC_LINK_PHY_ID, AC_LINK_PHY_FRAME_SEND_TO,
          LINK_PHY_FRAME_SEND_TO_INTERVAL, TIMER_ONE_SHOT);
/* rồi return luôn */
```

Không `while`, không `delay`. Có ACK thì `timer_remove_attr()` huỷ. Hết giờ thì signal `AC_LINK_PHY_FRAME_SEND_TO` tự tới.

Viết kiểu bare-metal thông thường thì sao? `while (!ack && timeout--) delay(1);` là CPU đứng im 250 ms. Game đơ, LED life ngừng nháy, watchdog có khi đá luôn.

### Hai task bàn giao việc cho nhau

`dungeon_action.cpp` giao cho `dungeon_state.cpp`:

```c
/* dungeon_action: tới lúc quái ra đòn */
dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_MONSTER_ATK_RESOLVE;
task_post_pure_msg(DUNGEON_STATE_ID, DUNGEON_STATE_RUN);
```

```c
/* dungeon_state: nhận việc, làm xong thì đẩy pha để giải phóng bên kia */
static void dungeon_monster_turn() {
    dungeon_enemy_action();
    dungeon_status_tick();
    ...
    dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_MONSTER_ATK_APPLY;
}
```

Chỗ đáng nói là pha `RESOLVE`.

`task_post` chỉ xếp hàng thôi, message được xử lý sau khi handler hiện tại return. Không có pha chờ riêng thì một tick tới sớm sẽ yêu cầu lượt quái lần thứ hai.

Đây là kiểu lỗi race đặc trưng của hệ message-driven. Cách xử lý là dựng hẳn một trạng thái chờ cho rõ ràng.

### Gộp công việc bằng timer

`common/screen_manager.cpp`:

```c
if (đã đủ 50ms) {
    view_render_screen(view_screen);          // vẽ ngay
} else {
    timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_RENDER_SCREEN,
              50 - time_diff, TIMER_ONE_SHOT);  // hẹn vẽ sau
}
```

`timer_set` với cùng cặp `(task, sig)` thay thế hẹn cũ. Nên nhiều message dồn trong 50 ms gộp lại thành đúng một lần vẽ.

Số đo thật, chạy harness dồn message trong 4 giây:

| | Số lần đẩy full-frame I2C |
|---|---|
| Không giới hạn | 446 |
| Có giới hạn | 73 |

Khung hình cuối giống hệt. Giảm 6 lần công việc I/O mà không mất gì.

## 3. Câu hỏi hay gặp

### AK có phải RTOS thật không? Real-time chỗ nào?

Trả lời thẳng luôn: AK là cooperative kernel kiểu active object, không phải RTOS preemptive như FreeRTOS.

Không có preemption, task chạy tới khi return. Không có stack riêng cho từng task. Không có `vTaskDelay()` kiểu chặn.

"Real-time" ở đây là soft real-time. Nó đảm bảo thứ tự ưu tiên khi lấy việc, và mọi việc đều có chặn trên về thời gian vì không ai được phép chặn.

Đổi lại thì RAM cực ít. 16 KB mà chạy được cả game, giao thức UART, với bootloader. Không cần mutex, không có deadlock do khoá.

### Không preemptive thì ưu tiên để làm gì?

Để quyết định thứ tự lấy ra khi nhiều message cùng chờ.

`task_timer_tick` ở mức 7 nên luôn được xử lý trước game ở mức 4. Timer nhờ vậy không trôi vì game bận.

Chỉ vào `ak/src/task.c:359`:

```c
while ((t_task_new = LOG2LKUP(task_ready)) > t_task_current) {
```

`LOG2LKUP(x)` là `32 - __builtin_clz(x)`, tìm bit 1 cao nhất trong một lệnh CPU, khỏi vòng lặp. Mẹo kinh điển của loại kernel này.

### Một task chạy lâu thì sao?

Mọi task khác phải chờ. Đây là giới hạn cố hữu của cooperative.

Cũng chính vì vậy mà trong code không có chỗ nào `delay()` hay `while` chờ điều kiện.

Chỗ lâu nhất hiện tại là `view_render_screen()`, đẩy 1024 byte qua I2C bit-bang. Nên mới đẻ ra cơ chế giới hạn 20 FPS ở mục 2.

Bị hỏi "làm sao đo" thì: bật `-DAPP_DBG_SIG_EN` hoặc `AK_TASK_LOG_CONSOLE_ENABLE`, kernel in ra `waitTime` với `exeTime` của từng message. Code ở `ak/src/task.c:421`.

### Dữ liệu chung giữa các task có cần mutex không?

Không. Mà có lý do đàng hoàng.

AK dispatch từng message một, trên một stack, không task nào cắt ngang task nào. `dungeon_runtime` được cả 5 task đọc ghi mà chẳng cần khoá.

Ghi chú này nằm ngay đầu `dungeon_runtime.h`.

Có một ngoại lệ: dữ liệu chia sẻ với ngắt thì vẫn cần `ENTRY_CRITICAL()`. Ví dụ `ak_timer_payload_irq` trong `timer.c`, ngắt SysTick ghi mà task đọc.

### Sao thứ tự post message lại quan trọng?

Vì 5 task game cùng mức ưu tiên 4, tức chung một hàng đợi FIFO, chạy đúng thứ tự post.

Trong `scr_dungeon_game.cpp`:

```c
case DUNGEON_TIME_TICK:
    task_post_pure_msg(DUNGEON_CONTROL_ID, DUNGEON_CONTROL_UPDATE);
    task_post_pure_msg(DUNGEON_EFFECT_ID,  DUNGEON_EFFECT_UPDATE);
    task_post_pure_msg(DUNGEON_LANE_ID,    DUNGEON_LANE_LEVEL_UP);
    task_post_pure_msg(DUNGEON_ACTION_ID,  DUNGEON_ACTION_RUN);
```

Hiệu ứng phải tàn trước khi máy pha đánh chạy tiếp. Đảo thứ tự là đổi hành vi game luôn.

### Hết message pool thì sao?

`get_pure_msg()` gọi `FATAL("MF", ...)`, xem `ak/src/message.c` dòng 70, 93, 103. Nó in UART, ghi fatal log vào W25Q80, rồi reset.

Nghĩa là không treo im lặng. Màn hình đứng thì cắm UART vào là thấy mã lỗi. Xem lại bằng lệnh shell `fatal`.

Pool hiện tại: 32 pure, 8 common (64 B), 8 dynamic (heap 128 B), 16 timer.

### Làm sao biết hệ thống còn sống?

Nhìn LED life nháy 1 giây.

Nó nháy trong `task_life`, xem `app/task_life.cpp:27`, cùng chỗ vỗ cả hai watchdog. Nên LED nháy nghĩa là kernel còn dispatch được message. LED đứng là có task nào đó không chịu return.

### Sao tới 2 watchdog?

IWDG là phần cứng, khoảng 30 s. Chạy bằng LSI riêng, không dính clock chính, không tắt được. Cứu được cả khi CPU loạn clock. Nhược điểm là reset im lặng, chết mà không biết vì sao.

Soft watchdog dùng TIM7, 20 s. Nó gọi `FATAL()` nên ghi log trước khi chết, biết chết ở đâu.

Một cái để chắc chắn sống lại. Một cái để biết vì sao chết.

### Sao không dùng FreeRTOS?

Chỉ có 16 KB SRAM thôi.

FreeRTOS mỗi task cần stack riêng. 10 task × 256 B đã là 2.5 KB chỉ riêng stack, chưa tính TCB với kernel. AK dùng chung một stack nên thêm task gần như không tốn gì.

Đổi lại là mất preemption. Mọi task phải tự giác return nhanh.

## 4. Chỗ còn hạn chế, nên tự nói ra trước

Nói trước bao giờ cũng hơn để bị hỏi.

### Ba signal của dungeon_state không làm gì

Đúng vậy. Cố ý, và có comment giải thích trong code.

Game này theo lượt. Khác archery mẫu, bên đó mũi tên bay, thiên thạch rơi, vụ nổ chạy, ba thứ độc lập cần tick song song. Ở đây vòng đời quái do `dungeon_lane` quản.

Ép việc cho chúng thì sinh lỗi thật. Nếu `STATE_SETUP` gọi `dungeon_set_monster_stats()` thì mỗi lần continue con quái đang bị thương sẽ hồi đầy máu.

Để rỗng kèm giải thích vẫn hơn làm bừa.

### tsm.c không ai dùng

Máy trạng thái dạng bảng đi kèm kernel, hiện chưa file nào dùng. `fsm.c` thì có, ở 3 tầng link.

### task_system không có

Signal `SYSTEM_AK_FLASH_UPDATE_REQ` khai trong `app/app.h` nhưng không task nào nhận.

Bản archery mẫu cũng vậy, cũng là code chết ở đó.

Cách thêm nằm ở [06-bootloader.md](06-bootloader.md#6-task_system-mảnh-còn-thiếu).

### Tài nguyên đang khá căng

| | Dùng | Tổng |
|---|---|---|
| Flash | khoảng 82 % | 116 KB |
| SRAM | khoảng 85 %, tính cả heap 2 KB | 16 KB |
| Stack | khoảng 2.3 KB | |

Frame lớn nhất là `fsm_link_state_handle` 496 B. Còn margin nhưng không nhiều.

Muốn giảm thì tắt `IF_LINK_UART_EN`, trả lại khoảng 1.5 KB SRAM từ pool PDU 4×384. Với điều kiện không cần nạp firmware qua UART.

## 5. Bị hỏi bất ngờ thì mở file nào

```raw
"Chỉ chỗ khởi tạo kernel"        -> app/app.cpp:93        main_app()
"Chỉ bảng task"                  -> app/task_list.cpp
"Chỉ scheduler"                  -> ak/src/task.c:352     task_sheduler()
"Chỉ chỗ post message"           -> ak/src/task.c:98      task_post()
"Chỉ nguồn nhịp"                 -> platform/stm32l/system.c:321  systick_handler()
"Chỉ software timer"             -> ak/src/timer.c        task_timer_tick()
"Chỉ chỗ nút bấm thành message"  -> app/app_bsp.cpp
"Chỉ FSM"                        -> networks/net/link/link_phy.cpp:104
"Chỉ state chung của game"       -> game/dungeon_game/dungeon_runtime.h
"Chỉ chỗ 2 task nói chuyện"      -> dungeon_action.cpp (post) -> dungeon_state.cpp (nhận)
"Chỉ bootloader quyết định"      -> boot/sources/app/app.cpp:52  boot_main()
```
