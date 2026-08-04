# 07 — Tầng giao thức UART

Thư mục `application/sources/networks/net/link/` là một giao thức truyền tin 3 tầng chạy trên UART. Thiết kế theo kiểu mô hình OSI thu nhỏ.

Bật tắt bằng cờ biên dịch:

```makefile
IF_LINK_OPTION = -DIF_LINK_UART_EN     # application/Makefile:49
```

Tắt đi thì ba task này biến khỏi bảng task luôn. Trong `task_list.h` chúng nằm trong `#if defined(IF_LINK_UART_EN)`.

## 1. Sao một cổng UART lại cần tới 3 tầng

UART thô chỉ cho mình một dòng byte. Không có ranh giới gói tin, không phát hiện lỗi, không xác nhận đã nhận, không có địa chỉ.

Mà nạp firmware qua UART thì phải có đủ mấy thứ đó. Thiếu là một byte nhiễu hỏng cả ảnh firmware.

```raw
┌─────────────────────────────────────────────────┐
│ LINK    (task_link, pri 5)                      │  gói tin ứng dụng, địa chỉ
├─────────────────────────────────────────────────┤
│ MAC     (task_link_mac, pri 4)                  │  gửi lại khi lỗi, quản trạng thái
├─────────────────────────────────────────────────┤
│ PHY     (task_link_phy, pri 3)                  │  đóng mở khung, checksum, ACK
├─────────────────────────────────────────────────┤
│ UART                                            │  byte thô
└─────────────────────────────────────────────────┘
```

Ưu tiên tăng dần từ dưới lên: PHY 3, MAC 4, LINK 5. Nghe ngược trực giác nhưng hợp lý. Tầng trên xử lý nhanh và ít việc, tầng dưới nhiều việc hơn nên để sau, khỏi chặn tầng trên.

Cả ba đều dùng `fsm.c` của kernel:

```c
fsm_dispatch(&fsm_link_phy, msg);   // link_phy.cpp:104
fsm_dispatch(&fsm_link_mac, msg);   // link_mac.cpp:93
fsm_dispatch(&fsm_link,     msg);   // link.cpp:36
```

Đây là chỗ duy nhất trong repo dùng FSM của kernel.

## 2. Tầng PHY

`link_phy.cpp`. Nó biến dòng byte thành khung có ranh giới.

### Bộ phân tích khung

```c
enum {
    PARSER_STATE_SOF = 0x00,   /* chờ byte bắt đầu khung */
    PARSER_STATE_DES_ADDR,     /* địa chỉ đích */
    PARSER_STATE_LEN,          /* độ dài payload */
    PARSER_STATE_DATA,         /* payload */
    PARSER_STATE_FCS           /* checksum */
};
```

Mỗi byte tới từ ngắt UART đẩy máy trạng thái đi một bước.

Nhận đủ khung mà checksum đúng thì post `AC_LINK_PHY_FRAME_REV` lên MAC. Sai thì `AC_LINK_PHY_FRAME_REV_CS_ERR`.

Kiểu phân tích từng byte này không chặn ai cả. Đúng tinh thần AK. Không có hàm nào kiểu `read_frame()` ngồi chờ.

### Loại khung

```c
PHY_FRAME_TYPE_ACK      = 0x01     /* khung xác nhận */
PHY_FRAME_SUB_TYPE_NACK_TO = 0x01  /* báo lỗi timeout */
```

### Timeout

```c
#define LINK_PHY_FRAME_SEND_TO_INTERVAL   250   /* ms chờ ACK */
#define LINK_PHY_FRAME_REV_TO_INTERVAL    250   /* ms chờ khung hoàn chỉnh */
#define LINK_PHY_MAX_RETRY_SET_DEFAULT      1   /* số lần gửi lại */
```

Cả hai timeout đều làm bằng `timer_set(..., TIMER_ONE_SHOT)`, không phải vòng đếm.

Gửi khung xong thì hẹn giờ rồi return luôn. Task rảnh đi làm việc khác. Có ACK thì `timer_remove_attr()` huỷ hẹn. Hết giờ mà chưa có thì signal timeout tự bắn tới.

Đây là ví dụ sạch nhất trong repo về chuyện chờ mà không chặn.

### Signal của PHY

```c
AC_LINK_PHY_INIT
AC_LINK_PHY_FRAME_SEND_REQ      /* MAC yêu cầu gửi */
AC_LINK_PHY_FRAME_SEND_TO       /* hết giờ chờ ACK */
AC_LINK_PHY_FRAME_REV           /* nhận được khung hợp lệ */
AC_LINK_PHY_FRAME_REV_TO        /* khung tới dở dang rồi im */
AC_LINK_PHY_FRAME_REV_CS_ERR    /* checksum sai */
```

## 3. Tầng MAC

`link_mac.cpp`. Ba trạng thái:

```raw
STATE_IDLE  ──send_req──>  STATE_SENDING  ──done──>  STATE_IDLE
     │                          │
     │                          └──err──> gửi lại (tối đa 1 lần) hoặc báo lỗi lên
     └──frame_rev──> STATE_RECEIVING ──> ráp xong ──> STATE_IDLE
```

```c
#define LINK_MAC_PDU_SENDING_RETRY_COUNTER_MAX  1
```

Signal:

```c
AC_LINK_MAC_INIT
AC_LINK_MAC_PHY_LAYER_STARTED    /* PHY báo đã sẵn sàng */
AC_LINK_MAC_FRAME_SEND_REQ
AC_LINK_MAC_FRAME_SEND_START
AC_LINK_MAC_FRAME_SEND_DONE
AC_LINK_MAC_FRAME_SEND_ERR
AC_LINK_MAC_FRAME_REV
AC_LINK_MAC_FRAME_REV_TO
```

Cái `AC_LINK_MAC_PHY_LAYER_STARTED` cho thấy các tầng khởi động theo thứ tự và báo nhau bằng message. PHY xong thì báo MAC, MAC xong thì báo LINK. Không tầng nào phải `delay()` ngồi chờ tầng dưới.

## 4. Tầng LINK

`link.cpp`. Quản lý PDU và địa chỉ.

```c
#define LINK_PDU_BUF_SIZE   384
#define LINK_PDU_POOL_SIZE  4

typedef struct link_pdu_t {
    uint32_t id;
    uint32_t is_used;
    uint32_t len;
    uint8_t  payload[LINK_PDU_BUF_SIZE];
} link_pdu_t;
```

4 × 384 = 1536 byte SRAM. Gần 10 % của 16 KB.

Nên nếu không dùng tính năng nạp firmware qua UART thì tắt `IF_LINK_UART_EN` đi cũng đáng cân nhắc.

API cấp phát:

```c
extern link_pdu_t* link_pdu_get(uint32_t);
extern void        link_pdu_free(uint32_t);
extern void        link_set_src_addr(uint32_t);
extern void        link_set_des_addr(uint32_t);
```

Pool tĩnh, cấp phát bằng cờ `is_used`. Không `malloc`, không phân mảnh. Cùng triết lý với message pool của AK.

## 5. Nối vào application

```raw
task_link  <──>  task_if  <──>  task_uart_if  <──>  UART
                    │
                    ▼
               task_fw (cập nhật firmware)
```

`task_if` là lớp trừu tượng "giao tiếp bên ngoài". `task_uart_if` là hiện thực cho UART. Muốn thêm BLE thì viết `task_ble_if`, khỏi đụng `task_if`.

Cả hai có 6 signal đối xứng:

```c
AC_IF_PURE_MSG_IN / OUT
AC_IF_COMMON_MSG_IN / OUT
AC_IF_DYNAMIC_MSG_IN / OUT
```

Nghĩa là message của AK đi xuyên qua dây UART sang thiết bị khác được. Task bên này post, task bên kia nhận, y như post nội bộ.

Đây là ý tưởng hay nhất của kiến trúc AK. Ranh giới thiết bị trở nên trong suốt với tầng ứng dụng.

## 6. Cấu hình log

`link_config.h`:

```c
#define LINK_PRINT_EN     1     /* thông báo mức cao */
#define LINK_DBG_SIG_EN   0     /* mọi signal, rất ồn */
#define LINK_DBG_DATA_EN  0     /* dump từng byte, cực ồn */
#define LINK_DBG_EN       0
```

Chỉ bật `LINK_DBG_DATA_EN` khi thật sự cần soi từng byte. Nó in mọi byte ra UART, tức là dùng chính đường truyền đang debug để in log.
