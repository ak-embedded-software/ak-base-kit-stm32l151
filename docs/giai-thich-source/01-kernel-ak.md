# 01 — Kernel AK

AK không phải RTOS preemptive như FreeRTOS. Nó là cooperative kernel kiểu active object.

Ý tưởng gốc lấy từ Miro Samek. Cái này ghi ngay trong comment đầu file `ak/src/task.c`.

Hiểu chỗ này là hiểu được phần lớn đồ án.

## 1. Khác FreeRTOS chỗ nào

| | FreeRTOS | AK |
|---|---|---|
| Mỗi task có stack riêng | Có | Không. Dùng chung một stack |
| Task bị cắt ngang giữa chừng | Có | Không. Chạy tới khi return |
| Task ngủ chờ | `vTaskDelay()`, block | Không block. Hẹn timer rồi return |
| Đơn vị công việc | Hàm chạy vô hạn `for(;;)` | Một message, xử lý xong thì thoát |
| Cần mutex | Thường xuyên | Hiếm. Không ai cắt ngang ai |

Từ đó suy ra một luật: **task không được phép chờ bận hay `delay()`**.

Làm vậy là cả hệ thống đứng. LED life ngừng nháy, watchdog không được vỗ, xong.

## 2. Task trông như thế nào

Task chỉ là một dòng trong bảng. Ở `app/task_list.cpp`:

```c
const task_t app_task_table[] = {
    {TASK_TIMER_TICK_ID , TASK_PRI_LEVEL_7, task_timer_tick },
    {AC_TASK_FW_ID      , TASK_PRI_LEVEL_2, task_fw         },
    {AC_TASK_SHELL_ID   , TASK_PRI_LEVEL_2, task_shell      },
    {AC_TASK_LIFE_ID    , TASK_PRI_LEVEL_6, task_life       },
    {AC_TASK_IF_ID      , TASK_PRI_LEVEL_4, task_if         },
    {AC_TASK_UART_IF_ID , TASK_PRI_LEVEL_4, task_uart_if    },
    {AC_TASK_DISPLAY_ID , TASK_PRI_LEVEL_4, task_display    },
    {DUNGEON_STATE_ID   , TASK_PRI_LEVEL_4, dungeon_state_handle   },
    {DUNGEON_LANE_ID    , TASK_PRI_LEVEL_4, dungeon_lane_handle    },
    {DUNGEON_CONTROL_ID , TASK_PRI_LEVEL_4, dungeon_control_handle },
    {DUNGEON_ACTION_ID  , TASK_PRI_LEVEL_4, dungeon_action_handle  },
    {DUNGEON_EFFECT_ID  , TASK_PRI_LEVEL_4, dungeon_effect_handle  },
    {DUNGEON_SCREEN_ID  , TASK_PRI_LEVEL_4, scr_dungeon_game_handle},
#if defined (IF_LINK_UART_EN)
    {AC_LINK_PHY_ID     , TASK_PRI_LEVEL_3, task_link_phy   },
    {AC_LINK_MAC_ID     , TASK_PRI_LEVEL_4, task_link_mac   },
    {AC_LINK_ID         , TASK_PRI_LEVEL_5, task_link       },
#endif
    {AK_TASK_EOT_ID     , TASK_PRI_LEVEL_0, (pf_task)0      }   // EOT = hết bảng
};
```

Tổng 16 task khi bật `IF_LINK_UART_EN`. Cộng thêm 1 polling task nữa.

Ba cột: ID (phải tăng dần vì dùng làm index), ưu tiên (1 tới 7), hàm xử lý.

Thân task lúc nào cũng dạng này:

```c
void dungeon_state_handle(ak_msg_t* msg) {
    switch (msg->sig) {
    case DUNGEON_STATE_RUN:
        dungeon_monster_turn();
        break;
    ...
    }
}
```

Nhận một message, làm việc, return. Không vòng lặp. Không chờ.

## 3. Scheduler chạy ra sao

Trong `ak/src/task.c`. Mỗi mức ưu tiên có một hàng đợi FIFO riêng:

```c
typedef struct {
    task_pri_t  pri;
    uint8_t     mask;     // bit đại diện mức này
    ak_msg_t*   qhead;
    ak_msg_t*   qtail;
} tcb_t;

static tcb_t   task_pri_queue[TASK_PRI_MAX_SIZE];  // 8 mức
static uint8_t task_ready;   // bitmap: mức nào đang có việc
```

### Lúc post message

`task_post`, dòng 98:

```c
t_tcb = &task_pri_queue[task_table[des_task_id].pri - 1];
ENTRY_CRITICAL();
    ...nối msg vào cuối hàng đợi của mức đó...
    task_ready |= t_tcb->mask;     // bật bit "mức này có việc"
EXIT_CRITICAL();
```

Để ý `pri - 1`. Nghĩa là ưu tiên phải từ 1 trở lên. `TASK_PRI_LEVEL_0` chỉ dùng cho dòng EOT, không ai post tới đó.

### Lúc chạy

`task_sheduler`, dòng 352:

```c
while ((t_task_new = LOG2LKUP(task_ready)) > t_task_current) {
    tcb_t* t_tcb = &task_pri_queue[t_task_new - 1];
    ak_msg_t* t_msg = t_tcb->qhead;      // lấy message đầu hàng
    t_tcb->qhead = t_msg->next;
    if (t_msg->next == AK_MSG_NULL) {
        t_tcb->qtail = AK_MSG_NULL;
        task_ready &= ~t_tcb->mask;      // hết việc thì tắt bit
    }
    ...
    EXIT_CRITICAL();
    task_table[t_msg->des_task_id].task(t_msg);   // <-- gọi task
    ENTRY_CRITICAL();
    msg_free(t_msg);                     // trả message về pool
}
```

`LOG2LKUP(x)` chính là `32 - __builtin_clz(x)`, định nghĩa ở `platform/stm32l/platform.h`. Nó trả về vị trí bit 1 cao nhất.

Mẹo này khá hay. Tìm mức ưu tiên cao nhất đang có việc chỉ tốn một lệnh CPU, khỏi cần vòng lặp quét.

Ví dụ `task_ready = 0b01010000` thì `LOG2LKUP` ra 7, chạy mức 7 trước.

### Mấy chỗ hay bị hỏi

Task ưu tiên thấp đang chạy, task ưu tiên cao có message thì sao?

Không cắt ngang được đâu. Task đang chạy vẫn chạy tới khi `return`. Thằng ưu tiên cao phải chờ vòng `while` kế tiếp. Cooperative là vậy.

Thế ưu tiên để làm gì?

Nó quyết định thứ tự lấy ra khi nhiều message cùng chờ. `task_timer_tick` ở mức 7 nên luôn được xử lý trước mấy task game ở mức 4. Timer nhờ vậy không bị trễ vì game bận.

Mấy task game cùng mức 4 thì sao?

Chung một hàng đợi FIFO, chạy đúng thứ tự post. Đây là lý do trong `scr_dungeon_game.cpp` thứ tự 4 dòng `task_post_pure_msg` lúc `DUNGEON_TIME_TICK` lại quan trọng, và có comment nhắc.

## 4. Message với 3 cái pool

Ở `ak/src/message.c`. Ba loại, cấp phát tĩnh từ mảng sẵn. Không malloc động cho message.

| Loại | Hàm post | Pool | Dùng khi |
|---|---|---|---|
| pure | `task_post_pure_msg(id, sig)` | 32 | Chỉ báo hiệu, không mang dữ liệu |
| common | `task_post_common_msg(id, sig, data, len)` | 8 msg × 64 byte | Ít dữ liệu, copy vào buffer cố định |
| dynamic | `task_post_dynamic_msg(id, sig, data, len)` | 8 msg, data lấy từ heap 128 byte | Dữ liệu dài, độ dài thay đổi |

Số mặc định ở `ak/inc/message.h:27-53`. Makefile override được.

Game chỉ xài pure msg. Mọi signal `DUNGEON_*` đều không mang dữ liệu, vì state chung đã nằm ở `dungeon_runtime` rồi.

### Ref count

`ak_msg_t.ref_count` nhét 2 thứ vào 1 byte:

```c
#define AK_MSG_TYPE_MASK       (0xC0)   // 2 bit cao : loại message
#define AK_MSG_REF_COUNT_MASK  (0x3F)   // 6 bit thấp: mấy task đang giữ
```

Một message post cho nhiều task được. `msg_free()` chỉ thật sự trả về pool khi ref count về 0.

### Hết pool thì sao

`get_pure_msg()` gọi `FATAL("MF", ...)`. Xem `message.c` dòng 70, 93, 103. Nó in ra UART, ghi fatal log vào W25Q80, rồi reset.

Nghĩa là không treo im lặng. Màn hình đứng mà cắm UART vào là thấy ngay.

## 5. Software timer

`ak/src/timer.c`, pool 16 cái (`AK_TIMER_POOL_SIZE`).

```c
timer_set(AC_TASK_DISPLAY_ID, DUNGEON_TIME_TICK, 100, TIMER_PERIODIC);
//        task nhận            signal sẽ bắn      ms   một lần / lặp
```

Chạy 2 tầng.

**Tầng dưới, trong ngắt SysTick.** `timer_tick(1)` chạy mỗi 1 ms:

```c
void timer_tick(uint32_t t) {
    if (timer_list_head != TIMER_MSG_NULL) {
        ak_timer_payload_irq.counter += t;             // cộng dồn
        if (ak_timer_payload_irq.enable_post_msg == AK_ENABLE) {
            ak_timer_payload_irq.enable_post_msg = AK_DISABLE;   // khoá
            ak_msg_t* s_msg = get_pure_msg();
            set_msg_sig(s_msg, TIMER_TICK);
            task_post(TASK_TIMER_TICK_ID, s_msg);      // post đúng 1 message
        }
    }
}
```

Cái cờ `enable_post_msg` làm nhiệm vụ giữ cho hàng đợi tối đa 1 message `TIMER_TICK`. Hệ thống bận 5 ms thì counter cộng lên 5, nhưng vẫn chỉ 1 message. Khỏi lo ngập pool vì ngắt bắn nhanh quá.

**Tầng trên, trong task.** `task_timer_tick`, ưu tiên 7:

```c
irq_counter = ak_timer_payload_irq.counter;   // lấy tổng ms đã trôi
ak_timer_payload_irq.counter = 0;
ak_timer_payload_irq.enable_post_msg = AK_ENABLE;   // mở khoá cho lần sau

while (timer_list != TIMER_MSG_NULL) {
    if (irq_counter < timer_list->counter) timer_list->counter -= irq_counter;
    else                                   timer_list->counter = 0;

    if (timer_list->counter == 0) {
        timer_msg = get_pure_msg();
        set_msg_sig(timer_msg, timer_list->sig);
        task_post(timer_list->des_task_id, timer_msg);   // bắn signal
        if (timer_list->period) timer_list->counter = timer_list->period;  // lặp
        else                    timer_del = timer_list;                    // one-shot thì xoá
    }
    timer_list = timer_list->next;
}
```

Nó trừ cả cụm ms một lần. Hệ thống có bận thì timer cũng không bị trôi tích luỹ.

Muốn huỷ timer đã hẹn thì `timer_remove_attr(task_id, sig)`. Chuyển màn hình hay dùng cái này.

## 6. FSM với TSM

Kernel có kèm 2 bộ máy trạng thái.

`fsm.c` là máy trạng thái phẳng. Trạng thái chính là con trỏ hàm, đổi trạng thái tức là gán con trỏ mới. Trong repo này chỉ có link layer dùng: `link_phy.cpp:104`, `link_mac.cpp:93`, `link.cpp:36`.

`tsm.c` là máy trạng thái dạng bảng. Hiện không file nào dùng.

Game không đụng tới `fsm.c`. Nó xài `screen_manager` (cùng ý tưởng con trỏ hàm nhưng viết riêng cho màn hình) và biến `battle_phase` trong `dungeon_runtime` cho máy trạng thái lượt đánh.

Nếu bị hỏi "FSM đâu" thì trả lời thẳng: FSM của kernel nằm ở tầng giao thức UART, còn game dùng 2 máy trạng thái tự viết. Xem thêm [05-game-dungeon.md](05-game-dungeon.md) mục 4.

## 7. Polling task

Ngoài task theo message còn có polling task. Loại này chạy khi kernel rảnh.

```c
const task_polling_t app_task_polling_table[] = {
    {AC_TASK_POLLING_CONSOLE_ID, AK_ENABLE, task_polling_console},
    {AK_TASK_POLLING_EOT_ID    , AK_DISABLE, (pf_task_polling)0 },
};
```

`task_run()` gọi luân phiên:

```c
for (;;) {
    task_sheduler();      // hết message thì thoát ra
    task_polling_run();   // rồi mới chạy polling
}
```

`task_polling_console()` ở `app/app.cpp:194` đọc ký tự UART từ ring buffer, gom thành dòng lệnh, post cho `task_shell`.

Để ở polling vì gõ phím không gấp. Không đáng chiếm chỗ trong hàng đợi ưu tiên.

## 8. Critical section

```c
#define ENTRY_CRITICAL()  entry_critical()   // platform/stm32l/platform.h
#define EXIT_CRITICAL()   exit_critical()
```

Cài đặt trong `platform/stm32l/system.c`. Tắt bật ngắt, có đếm lồng nhau qua `get_nest_entry_critical_counter()`, nên lồng nhiều tầng vẫn đúng.

Dùng ở mọi chỗ đụng vào hàng đợi hay pool. Lý do là ngắt SysTick có thể post message giữa chừng. Đó cũng là vì sao `task_post` phải bọc critical section, vì nó chạy được cả từ ngắt lẫn từ task.

## 9. Tóm tắt

```raw
SysTick 1ms ──> timer_tick()  ──> post TIMER_TICK (tối đa 1 cái)
                                        │
                                        ▼
                              task_timer_tick (pri 7)
                                        │  trừ counter mọi timer
                                        │  cái nào về 0 thì post signal
                                        ▼
                              task đích (pri 4...) xử lý
                                        │
                                        │  có thể post tiếp cho task khác
                                        ▼
                              ... tới khi hàng đợi rỗng
                                        │
                                        ▼
                              task_polling_run()  (đọc UART)
```
