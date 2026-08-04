# 06 — Bootloader

Bootloader là chương trình riêng. Biên dịch riêng, nạp riêng. Nằm ở thư mục `boot/`, chiếm 8 KB tại `0x08000000`.

Nó chạy trước application ở mọi lần reset.

## 1. Cần nó làm gì

Không có nó thì muốn nạp firmware mới phải cắm ST-Link. Có rồi thì chỉ cần cáp UART. Người dùng cuối tự cập nhật được.

Với lại đây cũng là chỗ đồ án khoe được nhiều thứ: chia vùng flash, checksum, cơ chế bắt tay, chế độ an toàn.

## 2. Nằm ở đâu

```raw
0x08000000  ┌──────────────────┐
            │  BOOTLOADER 8 KB │  boot/  <- chạy trước
0x08002000  ├──────────────────┤
            │  BSF        4 KB │  vùng bắt tay, hai bên cùng đọc ghi
0x08003000  ├──────────────────┤
            │  APPLICATION 116K│  application/
            └──────────────────┘
```

Bootloader có `ak.ld` riêng ở `boot/sources/platform/stm32l/ak.ld`, với `FLASH ORIGIN = 0x08000000, LENGTH = 8K`.

Nó không dùng kernel AK. Chỉ là code tuần tự đơn giản. Càng ít thứ càng ít chỗ hỏng.

## 3. Cấu trúc bắt tay

Trong `sys/sys_boot.h`:

```c
#define FIRMWARE_PSK  0x1A2B3C4D          /* magic, nghĩa là "vùng này hợp lệ" */

typedef struct {
    uint32_t psk;          /* phải == FIRMWARE_PSK */
    uint32_t checksum;
    uint32_t bin_len;
} firmware_header_t;

typedef struct {
    uint8_t  cmd;          /* NONE / UPDATE_REQ / UPDATE_RES */
    uint8_t  container;    /* firmware nằm ở đâu */
    uint8_t  io_driver;    /* nhận qua đường nào */
    uint32_t des_addr;     /* ghi vào đâu */
    uint32_t src_addr;     /* đọc từ đâu */
} firmware_boot_cmd_t;

typedef struct {
    firmware_header_t   current_fw_boot_header;
    firmware_header_t   update_fw_boot_header;
    firmware_boot_cmd_t fw_boot_cmd;
    firmware_header_t   current_fw_app_header;
    firmware_header_t   update_fw_app_header;
    firmware_boot_cmd_t fw_app_cmd;
    ...
} sys_boot_t;
```

Mấy hằng số:

```c
#define SYS_BOOT_CMD_NONE                   0x01   /* không có việc gì, chạy app */
#define SYS_BOOT_CMD_UPDATE_REQ             0x02   /* xin cập nhật */
#define SYS_BOOT_CMD_UPDATE_RES             0x03

#define SYS_BOOT_CONTAINER_DIRECTLY         0x01   /* nhận thẳng qua io driver */
#define SYS_BOOT_CONTAINER_EXTERNAL_FLASH   0x02   /* ảnh nằm sẵn ở W25Q80 */
#define SYS_BOOT_CONTAINER_INTERNAL_FLASH   0x03
#define SYS_BOOT_CONTAINER_EXTERNAL_EPPROM  0x04
#define SYS_BOOT_CONTAINER_INTERNAL_EPPROM  0x05
#define SYS_BOOT_CONTAINER_SDCARD           0x06

#define SYS_BOOT_IO_DRIVER_NONE             0x01
#define SYS_BOOT_IO_DRIVER_UART             0x02
#define SYS_BOOT_IO_DRIVER_SPI              0x03
```

Danh sách `CONTAINER` với `IO_DRIVER` dài hơn thực tế dùng. Thiết kế mở sẵn cho SD card, SPI các kiểu. Hiện chỉ `DIRECTLY` với `UART`, và `EXTERNAL_FLASH`, là có code.

## 4. Bootloader quyết định thế nào

`boot/sources/app/app.cpp:52`, hàm `boot_main()`:

```raw
        Reset
          │
          ▼
   sys_boot_init(); sys_boot_get(&app_sys_boot);   đọc BSF
          │
          ▼
   io_button_mode_init();
   uart_boot_init(uart_boot_cmd_handshake_res);
          │
          ▼
   ┌─────────────────────────────────────────────────┐
   │ uart_boot_is_required()                         │
   │   HOẶC                                          │
   │ fw_app_cmd.cmd == UPDATE_REQ                    │
   │   && container == DIRECTLY                      │
   │   && io_driver == UART                          │
   └────────────────┬────────────────────────────────┘
             có     │              không
          ┌─────────┘                  │
          ▼                            ▼
   CHẾ ĐỘ NẠP QUA UART        ┌──────────────────────────────┐
   led nháy 250ms             │ cmd == NONE                  │
   while(1) chờ lệnh          │  && current_fw_app_header    │
                              │     .psk == FIRMWARE_PSK     │
                              └───────┬──────────────────────┘
                                   có │        không
                                      ▼            ▼
                          jump_to_application()  chép firmware
                                                 từ W25Q80 vào
                                                 flash nội
```

### Giữ nút MODE để vào chế độ nạp

```c
uint32_t uart_boot_is_required() {
    for (int i = 0; i < 300; i++) {
        if (io_button_mode_read()) {   /* nhả ra lúc nào là thôi */
            return 0;
        }
    }
    return 1;                          /* giữ suốt 300 vòng thì vào chế độ nạp */
}
```

Đây là lối thoát hiểm.

Lỡ nạp nhầm firmware hỏng làm app crash liên tục, vẫn vào lại được chế độ nạp. Chỉ cần giữ nút MODE lúc cấp nguồn.

Để ý là 300 vòng lặp, không phải 300 ms. Thời gian thật phụ thuộc tần số clock. Thực tế thì rất ngắn, chỉ cần nút đang bị giữ ở thời điểm kiểm tra thôi.

### Kiểm app có hợp lệ không

```c
if (app_sys_boot.fw_app_cmd.cmd == SYS_BOOT_CMD_NONE &&
        app_sys_boot.current_fw_app_header.psk == FIRMWARE_PSK) {
    APP_PRINT("[BOOT] start application\n");
    jump_to_application_before_reset_peripheral();
}
```

Hai điều kiện: không có lệnh update đang chờ, và vùng app có magic hợp lệ.

Thiếu một trong hai thì không nhảy. Tránh nhảy vào flash trống rồi treo.

### Chuyển sang application

```c
void jump_to_application_before_reset_peripheral() {
    update_boot_fw_info_to_share_boot();

    sys_ctrl_jump_to_app_req = SYS_CTRL_JUMP_TO_APP_REQ;
    sys_ctrl_reset();                  /* <-- reset, không phải nhảy trực tiếp */

    while (1) {                        /* không bao giờ tới đây */
        led_life_on();  sys_ctrl_delay_ms(200);
        led_life_off(); sys_ctrl_delay_ms(200);
    }
}
```

Chỗ này hay. Nó reset chip chứ không nhảy thẳng tới `0x08003000`.

Vì sao lại tốt hơn. Nhảy thẳng thì mọi ngoại vi bootloader đã bật, tức UART, GPIO, timer, vẫn đang chạy. Ngắt vẫn đang bật. Vector table vẫn trỏ vùng boot. Application khởi động trên một môi trường bẩn như vậy sẽ sinh ra lỗi cực khó tìm.

Reset cứng thì chip về trạng thái sạch. Cờ `sys_ctrl_jump_to_app_req` nằm ở vùng không bị xoá, nó báo cho lượt boot kế tiếp biết là lần này nhảy thẳng sang app.

Vòng `while(1)` nháy LED phía sau là mã phòng thủ. Nếu reset không xảy ra thì LED nháy đều, báo có gì đó bất thường, chứ không im lặng.

### Chép firmware từ W25Q80

Khi `container == EXTERNAL_FLASH`, bootloader tự chép:

```c
FLASH_Unlock();
FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
                FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR | FLASH_FLAG_OPTVERRUSR);

internal_flash_erase_pages_cal(fw_app_cmd.des_addr, update_fw_app_header.bin_len);

led_blink_set(&led_life, 1000, 50);   /* LED báo đang ghi */

while (external_fw_index < update_fw_app_header.bin_len) {
    sys_ctrl_independent_watchdog_reset();       /* <-- quan trọng */
    flash_read(src_addr + external_fw_index, (uint8_t*)&temp, sizeof(uint32_t));
    flash_status = FLASH_FastProgramWord(des_addr + external_fw_index, temp);
    ...
}
```

Dòng `sys_ctrl_independent_watchdog_reset()` trong vòng lặp là bắt buộc. Xoá với ghi 116 KB flash mất khá lâu. Không vỗ watchdog là bị reset giữa chừng, để lại firmware dở dang.

## 5. Phía application

`app/task_fw.cpp` là đối tác của bootloader.

Nó nói chuyện với máy tính qua link layer, nhận từng gói, ghi vào W25Q80, rồi đặt lệnh vào BSF và reset.

```raw
FW_CHECKING_REQ                 hẹn 5 s sau khi khởi động, do app_start_timer
FW_CRENT_APP_FW_INFO_REQ        host hỏi phiên bản app
FW_CRENT_BOOT_FW_INFO_REQ       host hỏi phiên bản bootloader
FW_UPDATE_REQ                   host xin cập nhật
FW_UPDATE_SM_OK / SM_BUSY       trả lời đồng ý / đang bận
FW_TRANSFER_REQ                 nhận từng gói firmware
FW_PACKED_TIMEOUT               quá 5 s không thấy gói thì huỷ
FW_INTERNAL_UPDATE_APP_RES_OK   ghi xong app
FW_INTERNAL_UPDATE_BOOT_RES_OK  ghi xong bootloader
FW_SAFE_MODE_RES_OK             vào chế độ an toàn
```

`FW_PACKED_TIMEOUT` đáng chú ý. Cáp UART rớt giữa chừng thì việc cập nhật tự huỷ, chứ không treo mãi.

## 6. task_system, mảnh còn thiếu

Game archery mẫu có `task_system` với signal `SYSTEM_AK_FLASH_UPDATE_REQ`:

```c
void task_system(ak_msg_t* msg) {
    switch (msg->sig) {
    case SYSTEM_AK_FLASH_UPDATE_REQ: {
        sys_boot_t sb;
        sys_boot_get(&sb);
        sb.fw_app_cmd.cmd       = SYS_BOOT_CMD_UPDATE_REQ;
        sb.fw_app_cmd.container = SYS_BOOT_CONTAINER_DIRECTLY;
        sb.fw_app_cmd.io_driver = SYS_BOOT_IO_DRIVER_UART;
        sb.fw_app_cmd.des_addr  = APP_START_ADDR;
        sb.fw_app_cmd.src_addr  = 0;
        sys_boot_set(&sb);
        sys_ctrl_reset();
    } break;
    }
}
```

Dungeon không có task này. Signal `SYSTEM_AK_FLASH_UPDATE_REQ` vẫn khai trong `app/app.h` nhưng không task nào nhận.

Nói cho công bằng thì trong bản archery mẫu nó cũng là code chết. Grep cả repo không thấy ai post signal đó.

Muốn thêm cho demo trọn vẹn thì làm 4 bước:

1. Thêm `AC_TASK_SYSTEM_ID` vào `task_list.h`, đặt trước các task game
2. Thêm dòng vào `app_task_table` với `TASK_PRI_LEVEL_2`
3. Viết `task_system.cpp` như trên
4. Thêm lệnh shell gọi `task_post_pure_msg(AC_TASK_SYSTEM_ID, SYSTEM_AK_FLASH_UPDATE_REQ)`

Xong rồi thì gõ một lệnh trên UART là bo tự reset vào chế độ nạp. Demo được cả bootloader lẫn RTOS trong một thao tác.
